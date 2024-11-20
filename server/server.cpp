#include "server.h"
#include <format>
#include <boost/asio.hpp>
#include <>

prova::server::server(boost::asio::io_service& io, std::size_t port): _port(port), _io(io), _connection(io) {
    _connection.bind(std::format("tcp://*:{}", _port));
}
void prova::server::start(){
    _connection.async_receive(std::bind(&server::receive, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
}

void prova::server::receive(const boost::system::error_code& ec, azmq::message& msg, size_t bytes_transferred){
    if(!ec){
        // std::cout << std::format("Received {} bytes", bytes_transferred) << std::endl;

        std::string_view body{static_cast<const char*>(msg.data()), msg.size()};
        auto pos = body.find(':');
        if (pos == std::string::npos) {
            // TODO unexpected
            return;
        }

        std::string_view agent  = body.substr(0, pos);
        std::string_view source = body.substr(pos+1, body.size() - pos);

        if(msg.more()){
            _connection.async_receive(std::bind(&server::receive_more, this, std::string{agent}, std::string{source}, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        } else{
            // TODO unexpected
            return;
        }
    } else {
        std::cerr << "receive error: " << ec.message() << std::endl;
    }

}

void prova::server::receive_more(std::string agent, std::string source, const boost::system::error_code& ec, azmq::message& msg, size_t bytes_transferred){
    if(!ec){
        if(!msg.more()){
            _connection.async_receive(std::bind(&server::receive, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
            // std::cout << std::format("Received {} bytes from {}:{}", bytes_transferred, agent, source) << std::endl;
            process(agent, source, msg);
        } else{
            // TODO unexpected
            return;
        }
    } else {
        std::cerr << "receive error: " << ec.message() << std::endl;
    }
}
void prova::server::process(const std::string agent, const std::string source, const azmq::message& msg){
    std::string_view body{static_cast<const char*>(msg.data()), msg.size()};
    // std::cout << source << " " << body << std::endl;
    if(source == "audit"){
        store_audit_log(agent, body);
    }
}

void prova::server::store_audit_log(const std::string& agent, const std::string_view& log){
    // _audit.post(agent, log);
    prova::sysaudit::parser parser{log};
    parser.parse();
    prova::sysaudit::reporter reporter{agent};
    reporter.report(parser.entries());
}
