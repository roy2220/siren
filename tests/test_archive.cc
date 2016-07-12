#include "archive.h"
#include "stream.h"
#include "test.h"


namespace {

using namespace siren;


SIREN_TEST("serialize/deserialize structure")
{
    Stream s;
    Archive a(&s);

    struct Dummy {
        char c;
        int i;
        std::string s;

        bool operator==(const Dummy& rhs) const {
            return c == rhs.c && i == rhs.i && s == rhs.s;
        }

        SIREN_SERDES(c, i, s)
    };

    Dummy input{'a', 1, "hello"};
    a << input;
    a.flush();
    Dummy output;
    a >> output;
    a.flush();
    SIREN_TEST_ASSERT(output == input);
    SIREN_TEST_ASSERT(s.getDataSize() == 0);
}

}
