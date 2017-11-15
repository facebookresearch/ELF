#pragma once

#include <zmq.hpp>
#include <string>
#include <sstream>
#include <iomanip>
#include <functional>

namespace elf {

namespace distri {

// The following codes are from zguide: https://github.com/booksbyus/zguide
inline std::string s_recv (zmq::socket_t & socket) {
    zmq::message_t message;
    socket.recv(&message);

    return std::string(static_cast<char*>(message.data()), message.size());
}

//  Convert string to 0MQ string and send to socket
inline bool s_send (zmq::socket_t & socket, const std::string & string) {

    zmq::message_t message(string.size());
    memcpy (message.data(), string.data(), string.size());

    bool rc = socket.send (message);
    return (rc);
}

//  Sends string as 0MQ string, as multipart non-terminal
inline bool s_sendmore (zmq::socket_t & socket, const std::string & string) {
    zmq::message_t message(string.size());
    memcpy (message.data(), string.data(), string.size());

    bool rc = socket.send (message, ZMQ_SNDMORE);
    return (rc);
}

class ZMQReceiver {
public:
    ZMQReceiver(int port, bool use_ipv6) : context_(1)  {
        broker_.reset(new zmq::socket_t(context_, ZMQ_ROUTER));
        if (use_ipv6) {
            int ipv6 = 1;
            broker_->setsockopt(ZMQ_IPV6, &ipv6, sizeof(ipv6));
        }
        broker_->bind("tcp://*:" + std::to_string(port));
    }

    void send(const std::string &identity, const std::string& msg) {
        s_sendmore(*broker_, identity);
        s_sendmore(*broker_, "");

        //  Encourage workers until it's time to fire them
        s_send(*broker_, msg);
    }

    void recv(std::string *identity, std::string *msg) {
        *identity = s_recv(*broker_);
        s_recv(*broker_);     //  Envelope delimiter
        *msg = s_recv(*broker_);     //  Response from worker
    }

    ~ZMQReceiver() { broker_.reset(nullptr); }

private:
    zmq::context_t context_;
    std::unique_ptr<zmq::socket_t> broker_;
};

class ZMQSender {
public:
    ZMQSender(const std::string &id, const std::string &addr, int port, bool use_ipv6)
      : context_(1)  {
        sender_.reset(new zmq::socket_t(context_, ZMQ_DEALER));
        if (use_ipv6) {
            int ipv6 = 1;
            sender_->setsockopt(ZMQ_IPV6, &ipv6, sizeof(ipv6));
        }
        sender_->setsockopt(ZMQ_IDENTITY, id.c_str(), id.length());
        sender_->connect("tcp://" + addr + ":" + std::to_string(port));
    }

    void send(const std::string& msg) {
        s_sendmore(*sender_, "");
        s_send(*sender_, msg);
    }

    std::string recv() {
        s_recv(*sender_);            //  Envelope delimiter
        return s_recv(*sender_);     //  Response from worker
    }

    ~ZMQSender() { sender_.reset(nullptr); }

private:
    zmq::context_t context_;
    std::unique_ptr<zmq::socket_t> sender_;
};

}  // namespace distri

}  // namespace elf

