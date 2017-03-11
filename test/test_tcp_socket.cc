#include <cstring>

#include "ip_endpoint.h"
#include "loop.h"
#include "stream.h"
#include "tcp_socket.h"
#include "test.h"


namespace {

using namespace siren;


SIREN_TEST("Ping-Pong sample")
{
    Loop l;
    TCPSocket ss(&l);
    ss.setReuseAddress(true);
    ss.listen(IPEndpoint(0, 0));
    IPEndpoint le = ss.getLocalEndpoint();

    l.createFiber([&] () -> void {
        TCPSocket cs = ss.accept();

        {
            char request[100];
            cs.read(request, sizeof(request));
            SIREN_TEST_ASSERT(std::strcmp(request, "ping!") == 0);
        }

        {
            const char reply[] = "pong!";
            cs.write(reply, sizeof(reply));
            cs.closeWrite();
        }

    }, 16 * 1024);

    l.createFiber([&] () -> void {
        TCPSocket cs(&l);
        cs.connect(le);

        {
            const char request[] = "ping!";
            cs.write(request, sizeof(request));
            cs.closeWrite();
        }

        {
            char reply[100];
            cs.read(reply, sizeof(reply));
            SIREN_TEST_ASSERT(std::strcmp(reply, "pong!") == 0);
        }
    }, 16 * 1024);

    l.run();
}


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
            cs.setReceiveTimeout(100);
            while (cs.read(&s) >= 1);
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

