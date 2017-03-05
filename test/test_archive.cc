#include "archive.h"
#include "stream.h"
#include "test.h"


namespace {

using namespace siren;


SIREN_TEST("Serialize/Deserialize structures")
{
    enum class Flag : unsigned int {
        A,
        B,
    };

    struct Dummy {
        char c;
        short si;
        int i;
        long li;
        std::string s;
        Flag f;

        bool operator==(const Dummy& rhs) const {
            return c == rhs.c &&
                   si == rhs.si &&
                   i == rhs.i &&
                   li == rhs.li &&
                   s == rhs.s &&
                   f == rhs.f;
        }

        SIREN_SERDES(c, si, i, li, s, f)
    };

    Stream s;
    Archive a(&s);
    Dummy input{'a', 1, -2, 3, "hello", Flag::B};
    Dummy output;

    a << input;
    s.commitData(a.getNumberOfPreWrittenBytes());
    SIREN_TEST_ASSERT(s.getDataSize() >= 1);
    a >> output;
    s.discardData(a.getNumberOfPreReadBytes());
    SIREN_TEST_ASSERT(s.getDataSize() == 0);
    SIREN_TEST_ASSERT(output == input);
}


SIREN_TEST("Serialize/Deserialize arrays")
{
    Stream s;
    Archive a(&s);
    int in[3] = {123, 234, 456};
    a << in;
    s.commitData(a.getNumberOfPreWrittenBytes());
    SIREN_TEST_ASSERT(s.getDataSize() >= 1);
    int out[3];
    a >> out;
    s.discardData(a.getNumberOfPreReadBytes());
    SIREN_TEST_ASSERT(s.getDataSize() == 0);

    for (int i = 0; i < 3; ++i) {
        SIREN_TEST_ASSERT(out[i] == in[i]);
    }
}


SIREN_TEST("Serialize/Deserialize vectors")
{
    Stream s;
    Archive a(&s);
    std::vector<int> in = {123, 234, 456};
    a << in;
    s.commitData(a.getNumberOfPreWrittenBytes());
    SIREN_TEST_ASSERT(s.getDataSize() >= 1);
    std::vector<int> out;
    a >> out;
    s.discardData(a.getNumberOfPreReadBytes());
    SIREN_TEST_ASSERT(s.getDataSize() == 0);
    SIREN_TEST_ASSERT(out.size() == in.size());

    for (int i = 0; i < (int)in.size(); ++i) {
        SIREN_TEST_ASSERT(out[i] == in[i]);
    }
}


SIREN_TEST("Serialize/Deserialize variable-length integers")
{
    Stream s;
    Archive a(&s);
    a << VLI<int>(-6);
    s.commitData(a.getNumberOfPreWrittenBytes());
    SIREN_TEST_ASSERT(s.getDataSize() == 1);
    VLI<int> t;
    a >> t;
    s.discardData(a.getNumberOfPreReadBytes());
    SIREN_TEST_ASSERT(s.getDataSize() == 0);
    SIREN_TEST_ASSERT(t == -6);
}

}
