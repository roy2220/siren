#include <cstring>

#include "async.h"
#include "ip_endpoint.h"
#include "loop.h"
#include "stream.h"
#include "tcp_socket.h"
#include "test.h"


namespace {

using namespace siren;


SIREN_TEST("TCP echo client/server")
{
    Loop l;

    l.createFiber([&] () -> void {
        TCPSocket ss(&l);
        ss.setReuseAddress(true);
        ss.listen(IPEndpoint(0, 0));
        IPEndpoint ipe = ss.getLocalEndpoint();

        l.createFiber([&] () -> void {
            TCPSocket cs(&l);
            cs.connect(ipe);
            Stream s;
            s.reserveBuffer(100);
            char m[] = "hello, word!";
            s.write(m, sizeof(m));

            do {
                cs.write(&s);
            } while(s.getDataSize() >= 1);

            cs.closeWrite();
            while (cs.read(&s, 100) >= 1);
            SIREN_TEST_ASSERT(std::strcmp(static_cast<char *>(s.getData()), m) == 0);
        });

        TCPSocket cs = ss.accept();
        cs.setNoDelay(true);
        Stream s;
        s.reserveBuffer(100);
        while (cs.read(&s) >= 1);

        do {
            l.usleep(50 * 1000);
            cs.write(&s);
        } while(s.getDataSize() >= 1);

        cs.closeWrite();
    });

    l.run();
}

}

