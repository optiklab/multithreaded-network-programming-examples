#include <cstdlib>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

// To compile:
// g++ -lboost_system server.cpp -o server

// To run:
// ./server 1500

// To connect:
// telnet 127.0.0.1 1500

class session
{
private:
    tcp::socket socket_;
    enum { max_length = 1024 };
    char data_[max_length];
public:
    session(boost::asio::io_service &io_service) : socket_(io_service) {}
    
    tcp::socket &socket() { return socket_; }
    
    void start()
    {
        socket_.async_read_some(
            boost::asio::buffer(data_, max_length),
            boost::bind(
                &session::handle_read,
                this,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    }

    void handle_write(const boost::system::error_code &error)
    {
        if (!error)
        {
            socket_.async_read_some(
                boost::asio::buffer(data_, max_length),
                boost::bind(
                    &session::handle_read,
                    this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
        }
        else
        {
            delete this; // clear session, because socket created in stack (not heap)
        }
    }
    
    void handle_read(const boost::system::error_code &error,
        size_t bytes_transferred)
    {
        if (!error)
        {
            boost::asio::async_write(
                socket_,
                boost::asio::buffer(data_, bytes_transferred),
                boost::bind(
                    &session::handle_write,
                    this,
                    boost::asio::placeholders::error));
        }
        else
        {
            delete this; // clear session, because socket created in stack (not heap)
        }
    }
};

class server
{
private:
    boost::asio::io_service &io_service_;
    tcp::acceptor acceptor_;
    
public:
    server(boost::asio::io_service &io_service, short port) :
        io_service_(io_service),
        acceptor_(io_service, tcp::endpoint(tcp::v4(), port))
    {
        session *new_session = new session(io_service_);
        acceptor_.async_accept(
            new_session->socket(),
            boost::bind(
                &server::handle_accept,
                this,
                new_session,
                boost::asio::placeholders::error));
    }
    
    void handle_accept(session *new_session, const boost::system::error_code &error)
    {
        if (!error)
        {
            new_session->start(); // Current session runs and continue working while not delete itself.
            new_session = new session(io_service_); // Create new session.
        }
        else
        {
            delete new_session;
        }
    }
};

int main(int argc, char** argv)
{
    boost::asio::io_service io_service;
    using namespace std;
    
    server s(io_service, atoi(argv[1]));
    
    printf("Server based on Boost::Asio PID -- %d.\n", getpid());
    
    io_service.run();
    
    return 0;
}