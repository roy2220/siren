#include "elf_hook.h"

#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <system_error>

#include <dlfcn.h>
#include <elf.h>
#include <fcntl.h>
#include <link.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "scope_guard.h"


#if defined(__i386__)
#  define ELF_R_SYM ELF32_R_SYM
#  define Elf_Ehdr Elf32_Ehdr
#  define Elf_Rel Elf32_Rel
#  define Elf_Shdr Elf32_Shdr
#  define Elf_Sym Elf32_Sym
#  define Elf_Word Elf32_Word
#  define REL_PLT ".rel.plt"
#elif defined(__x86_64__)
#  define ELF_R_SYM ELF64_R_SYM
#  define Elf_Ehdr Elf64_Ehdr
#  define Elf_Rel Elf64_Rela
#  define Elf_Shdr Elf64_Shdr
#  define Elf_Sym Elf64_Sym
#  define Elf_Word Elf64_Word
#  define REL_PLT ".rela.plt"
#else
#  error architecture not supported
#endif


namespace siren {

namespace {

char *GetModule(const char **);
const char *MapFile(const char *, std::size_t *);
void UnmapFile(const char *, std::size_t);
const Elf_Shdr *FindELFSectionHeaderByType(const char *, Elf_Word, std::size_t *);
const Elf_Shdr *FindELFSectionHeaderByName(const char *, const char *, std::size_t *);
const Elf_Sym *FindELFSymbolByName(const char *, const Elf_Shdr *, const char *, std::size_t *);

} // namespace


DLError::DLError() noexcept
  : description_(dlerror())
{
}


namespace detail {

std::uintptr_t *
LocateFunctionPointer(const char *elfFileName, const char *functionName)
{
    char *elfModule = GetModule(&elfFileName);
    std::size_t elfFileSize;
    const char *elfFileData = MapFile(elfFileName, &elfFileSize);

    auto scopeGuard = MakeScopeGuard([&] {
        UnmapFile(elfFileData, elfFileSize);
    });

    const Elf_Shdr *dynSym = FindELFSectionHeaderByType(elfFileData, SHT_DYNSYM, nullptr);
    std::size_t functionNameIndex;
    FindELFSymbolByName(elfFileData, dynSym, functionName, &functionNameIndex);
    const Elf_Shdr *relPlt = FindELFSectionHeaderByName(elfFileData, REL_PLT, nullptr);
    auto elfRelocations = reinterpret_cast<const Elf_Rel *>(elfModule + relPlt->sh_addr);
    std::size_t n = relPlt->sh_size / sizeof(Elf_Rel);

    for (std::size_t i = 0; i < n; ++i) {
        const Elf_Rel *elfRelocation = &elfRelocations[i];

        if (ELF_R_SYM(elfRelocation->r_info) == functionNameIndex) {
            auto FunctionPtr = reinterpret_cast<std::uintptr_t *>(elfModule
                                                                  + elfRelocation->r_offset);
            return FunctionPtr;
        }
    }

    throw std::invalid_argument("elf relocation not found");
}

} // namespace detail


namespace {

char *
GetModule(const char **fileName)
{
    void *moduleHandle = dlopen(*fileName, RTLD_LAZY | RTLD_NOLOAD);

    if (moduleHandle == nullptr) {
        throw DLError();
    }

    auto scopeGuard = MakeScopeGuard([&] {
        dlclose(moduleHandle);
    });

    const link_map *linkMap;

    if (dlinfo(moduleHandle, RTLD_DI_LINKMAP, &linkMap) < 0) {
        throw DLError();
    }

    if (std::strlen(linkMap->l_name) == 0) {
        *fileName = "/proc/self/exe";
    } else {
        *fileName = linkMap->l_name;
    }

    auto module = reinterpret_cast<char *>(linkMap->l_addr);
    return module;
}


const char *
MapFile(const char *fileName, std::size_t *fileSize)
{
    int fd = open(fileName, O_RDONLY);

    if (fd < 0) {
        throw std::system_error(errno, std::system_category(), "open() failed");
    }

    auto scopeGuard = MakeScopeGuard([&] {
        close(fd);
    });

    struct stat stat;

    if (fstat(fd, &stat) < 0) {
        throw std::system_error(errno, std::system_category(), "fstat() failed");
    }

    *fileSize = stat.st_size;
    auto fileData = reinterpret_cast<const char *>(mmap(nullptr, *fileSize, PROT_READ, MAP_PRIVATE
                                                        , fd, 0));

    if (fileData == nullptr) {
        throw std::system_error(errno, std::system_category(), "mmap() failed");
    }

    return fileData;
}


void
UnmapFile(const char *fileData, std::size_t fileSize)
{
    if (munmap(const_cast<char *>(fileData), fileSize) < 0) {
        throw std::system_error(errno, std::system_category(), "munmap() failed");
    }
}


const Elf_Shdr *
FindELFSectionHeaderByType(const char *elfFileData, Elf_Word elfSectionType
                           , std::size_t *elfSectionIndex)
{
    auto elfHeader = reinterpret_cast<const Elf_Ehdr *>(elfFileData);
    auto elfSectionHeaders = reinterpret_cast<const Elf_Shdr *>(elfFileData + elfHeader->e_shoff);

    for (std::size_t i = 0; i < elfHeader->e_shnum; ++i) {
        const Elf_Shdr *elfSectionHeader = &elfSectionHeaders[i];

        if (elfSectionHeader->sh_type == elfSectionType) {
            if (elfSectionIndex != nullptr) {
                *elfSectionIndex = i;
            }

            return elfSectionHeader;
        }
    }

    throw std::invalid_argument("elf section not found");
}


const Elf_Shdr *
FindELFSectionHeaderByName(const char *elfFileData, const char *elfSectionName
                           , std::size_t *elfSectionIndex)
{
    auto elfHeader = reinterpret_cast<const Elf_Ehdr *>(elfFileData);
    auto elfSectionHeaders = reinterpret_cast<const Elf_Shdr *>(elfFileData + elfHeader->e_shoff);

    if (elfHeader->e_shstrndx == SHN_UNDEF) {
        throw std::invalid_argument("elf section name string table undefined");
    }

    const Elf_Shdr *stringSectionHeader;

    if (elfHeader->e_shstrndx == SHN_XINDEX) {
        stringSectionHeader = &elfSectionHeaders[elfSectionHeaders->sh_link];
    } else{
        stringSectionHeader = &elfSectionHeaders[elfHeader->e_shstrndx];
    }

    const char *strings = elfFileData + stringSectionHeader->sh_offset;

    for (std::size_t i = 0; i < elfHeader->e_shnum; ++i) {
        const Elf_Shdr *elfSectionHeader = &elfSectionHeaders[i];

        if (std::strcmp(&strings[elfSectionHeader->sh_name], elfSectionName) == 0) {
            if (elfSectionIndex != nullptr) {
                *elfSectionIndex = i;
            }

            return elfSectionHeader;
        }
    }

    throw std::invalid_argument("elf section not found");
}


const Elf_Sym *
FindELFSymbolByName(const char *elfFileData, const Elf_Shdr *elfSectionHeader
                    , const char *elfSymbolName, std::size_t *elfSymbolIndex)
{
    auto elfHeader = reinterpret_cast<const Elf_Ehdr *>(elfFileData);
    auto elfSectionHeaders = reinterpret_cast<const Elf_Shdr *>(elfFileData + elfHeader->e_shoff);
    const Elf_Shdr *stringSectionHeader = &elfSectionHeaders[elfSectionHeader->sh_link];
    const char *strings = elfFileData + stringSectionHeader->sh_offset;
    auto elfSymbols = reinterpret_cast<const Elf_Sym *>(elfFileData + elfSectionHeader->sh_offset);
    std::size_t n = elfSectionHeader->sh_size / sizeof(Elf_Sym);

    for (std::size_t i = 0; i < n; ++i) {
        const Elf_Sym *elfSymbol = &elfSymbols[i];

        if (std::strcmp(&strings[elfSymbol->st_name], elfSymbolName) == 0) {
            if (elfSymbolIndex != nullptr) {
                *elfSymbolIndex = i;
            }

            return elfSymbol;
        }
    }

    throw std::invalid_argument("elf symbol not found");
}

} // namespace

} // namespace siren
