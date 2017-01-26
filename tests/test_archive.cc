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
    SIREN_TEST_ASSERT(s.getDataSize() == 0);
    Dummy input{'a', 1, -2, 3, "hello", Flag::B};
    Archive(&s) << input;
    SIREN_TEST_ASSERT(s.getDataSize() >= 1);
    Dummy output;
    Archive(&s) >> output;
    SIREN_TEST_ASSERT(s.getDataSize() == 0);
    SIREN_TEST_ASSERT(output == input);
}


SIREN_TEST("Serialize/Deserialize arrays")
{
    Stream s;
    int in[3] = {123, 234, 456};
    Archive(&s) << in;
    int out[3];
    Archive(&s) >> out;

    for (int i = 0; i < 3; ++i) {
        SIREN_TEST_ASSERT(out[i] == in[i]);
    }
}


SIREN_TEST("Serialize/Deserialize vectors")
{
    Stream s;
    Archive a(&s);
    std::vector<int> in = {123, 234, 456};
    Archive(&s) << in;
    std::vector<int> out;
    Archive(&s) >> out;
    SIREN_TEST_ASSERT(out.size() == in.size());

    for (int i = 0; i < (int)in.size(); ++i) {
        SIREN_TEST_ASSERT(out[i] == in[i]);
    }
}

}
