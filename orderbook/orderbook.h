#pragma once
#include <iostream>
#include <map>
#include <list>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <functional>
#include <chrono>
using namespace std;

static constexpr size_t CLSZ = 64;

struct alignas(CLSZ) Order {
    uint64_t id;
    bool buy;
    double p;
    uint64_t q;
    uint64_t ts;
};

struct LvlP {
    double p;
    uint64_t q;
    LvlP() {}
    LvlP(double _p, uint64_t _q) : p(_p), q(_q) {}
};

class alignas(CLSZ) Pool {
private:
    static const size_t BSZ = 1024;
    vector<Order*> blks;
    vector<Order*> free_q;
public:
    ~Pool();
    Order* alloc();
    void free(Order* o);
};

struct alignas(32) Lvl {
    list<Order*> q;
    uint64_t qty;
    double p;
    Lvl() : qty(0), p(0.0) {}
    explicit Lvl(double _p) : qty(0), p(_p) {}
};

struct Ref {
    Order* o;
    list<Order*>::iterator it;
    map<double, Lvl, function<bool(double,double)>>* bk;
    double p;
};

struct alignas(32) Trade {
    uint64_t b;
    uint64_t s;
    double p;
    uint64_t q;
    uint64_t t;
};

class alignas(CLSZ) Book {
public:
    Book();
    void add(const Order& o);
    bool cancel(uint64_t id);
    bool mod(uint64_t id, double np, uint64_t nq);
    void snap(size_t d, vector<LvlP>& bs, vector<LvlP>& as) const;
    void print(size_t d = 10) const;
    const vector<Trade>& get_ts() const { return ts; }
    void clr_ts() { ts.clear(); }

private:
    alignas(CLSZ) map<double,Lvl,function<bool(double,double)>> bids;
    alignas(CLSZ) map<double,Lvl,function<bool(double,double)>> asks;
    alignas(CLSZ) unordered_map<uint64_t,Ref> ref;
    alignas(CLSZ) Pool pool;
    alignas(CLSZ) vector<Trade> ts;

    map<double,Lvl,function<bool(double,double)>>& side(bool buy);
    void match();
};

uint64_t now_ns();