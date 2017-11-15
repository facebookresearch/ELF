/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#pragma once

#include <time.h>
#include <iostream>
#include <functional>
#include <chrono>
#include <random>
#include <sstream>
#include <thread>
#include "primitive.h"
#include "zmq_util.h"
#include <unistd.h>

namespace elf {

namespace shared {

using namespace std;

class Writer {
public:
    // Constructor.
    Writer(const std::string & addr, bool use_ipv6, int port = 5556)
      : rng_(time(NULL)) {
        identity_ = get_id(rng_);
        sender_.reset(new elf::distri::ZMQSender(identity_, addr, port, use_ipv6));
    }

    bool Insert(const std::string &s) {
        sender_->send(s);
        return true;
    }

private:
   unique_ptr<elf::distri::ZMQSender> sender_;
   mt19937 rng_;
   string identity_;

   static string get_id(mt19937 &rng) {
       char hostname[HOST_NAME_MAX];
       gethostname(hostname, HOST_NAME_MAX);

       stringstream ss;
       ss << hostname;
       ss << hex;
       for (int i = 0; i < 4; ++i) {
           ss << "-";
           ss << (rng() & 0xffff);
       }
       return ss.str();
   }
};

class Reader {
public:
    class Sampler {
    public:
        explicit Sampler(Reader *data) : data_(data), rng_(time(NULL)) {
            lock();
        }
        Sampler(const Sampler &) = delete;
        Sampler(Sampler &&sampler) : data_(sampler.data_), rng_(move(sampler.rng_)) { }

        const std::string& sample() {
            const vector<string> *loaded = &data_->get_recent_load();
            while (loaded->size() < 10) {
                unlock();
                this_thread::sleep_for(chrono::milliseconds(100));
                lock();
                loaded = &data_->get_recent_load();
                if (data_->verbose_) {
                    std::cout << "#Loaded: " << loaded->size() << std::endl;
                }
            }

            int idx = rng_() % loaded->size();
            return loaded->at(idx);
        }

        ~Sampler() { unlock(); }

    private:
        Reader *data_;
        mt19937 rng_;

        void lock() {
            data_->rw_lock_.read_shared_lock();
        }
        void unlock() {
            data_->rw_lock_.read_shared_unlock();
        }
    };

public:
    Reader(bool use_ipv6, int port = 5556) : receiver_(port, use_ipv6) {
        receiver_thread_.reset(new thread([](Reader *reader) {
            string identity, msg;
            while (true) {
                reader->receiver_.recv(&identity, &msg);
                reader->rw_lock_.write_lock();
                reader->buffer_.push_back(msg);
                reader->rw_lock_.write_unlock();
            }
        }, this));
    }

    Sampler GetSampler() {
        return Sampler(this);
    }


private:
    elf::distri::ZMQReceiver receiver_;
    unique_ptr<thread> receiver_thread_;
    bool verbose_ = false;

    mutable RWLock rw_lock_;
    vector<string> buffer_;

    const vector<string> &get_recent_load() const { return buffer_; }
};

}  // namespace shared

} // namespace elf
