#pragma once

namespace test_lifetime {

struct InstanceCounter {
    static int instances;
    InstanceCounter() { ++instances; }
    InstanceCounter(const InstanceCounter &) { ++instances; }
    InstanceCounter & operator=(const InstanceCounter &) { ++instances; return *this;}
    ~InstanceCounter() { --instances; }
};
int InstanceCounter::instances = 0;

}
