#include <boost/asio.hpp>
#include "sysaudit/parser.h"
#include "sysaudit/reporter.h"
#include "server.h"

int main() {
    boost::asio::io_service io;
    prova::server server{io, 9999};
    server.start();

    io.run();

    return 0;
}
