// SPDX-FileCopyrightText: 2024 Sunanda Bose <sunanda@simula.no>
// SPDX-License-Identifier: BSD-3-Clause

#ifndef PROVA_SERVER_H
#define PROVA_SERVER_H

#include <string>
#include <boost/asio.hpp>
#include <azmq/socket.hpp>

namespace prova{

struct server{
    server(boost::asio::io_service& io, std::size_t port);
    void start();
    private:
        void receive(const boost::system::error_code& ec, azmq::message& msg, size_t bytes_transferred);
        void receive_more(std::string agent, std::string source, const boost::system::error_code& ec, azmq::message& msg, size_t bytes_transferred);
        void process(const std::string agent, const std::string source, const azmq::message& msg);
        /**
         * @brief parses a single audit log message from log and stores it into a std::map
         */
        void store_audit_log(const std::string& agent, const std::string_view& log);
    private:
        std::size_t              _port;
        boost::asio::io_service& _io;
        azmq::pull_socket        _connection;
};

}

#endif // PROVA_SERVER_H
