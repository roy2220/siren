#include "archive.h"
#include "stream.h"
#include "test.h"


namespace {

using namespace siren;


SIREN_TEST("Serialize/Deserialize structures")
{
    enum class Flag : unsigned int {
        A,
        B
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
    Dummy input{'a', 1, 2, 3, "hello", Flag::B};
    a << input;
    a.flush();
    Dummy output;
    a >> output;
    a.flush();
    SIREN_TEST_ASSERT(output == input);
    SIREN_TEST_ASSERT(s.getDataSize() == 0);
}

}
