#include "order_book.h"
using namespace std;

uint64_t now_ns() {
    auto n = chrono::high_resolution_clock::now();
    return chrono::duration_cast<chrono::nanoseconds>(n.time_since_epoch()).count();
}

Pool::~Pool() {
    for (auto b : blks) delete[] b;
}

Order* Pool::alloc() {
    if (free_q.empty()) {
        Order* b = new Order[BSZ];
        blks.push_back(b);
        for (size_t i = 0; i < BSZ; i++) free_q.push_back(&b[i]);
    }
    auto o = free_q.back();
    free_q.pop_back();
    return o;
}

void Pool::free(Order* o) {
    free_q.push_back(o);
}

Book::Book()
    : bids([](double a,double b){return a>b;}),
      asks([](double a,double b){return a<b;}) {}

map<double,Lvl,function<bool(double,double)>>& Book::side(bool buy) {
    return buy ? bids : asks;
}

void Book::add(const Order& o) {
    auto& bk = side(o.buy);
    auto it = bk.find(o.p);
    if (it == bk.end()) it = bk.emplace(o.p, Lvl(o.p)).first;

    auto* x = pool.alloc();
    *x = o;
    it->second.q.push_back(x);
    it->second.qty += o.q;

    auto lit = prev(it->second.q.end());
    Ref r{x, lit, &bk, o.p};
    ref[o.id] = r;
    match();
}

bool Book::cancel(uint64_t id) {
    auto f = ref.find(id);
    if (f == ref.end()) return false;
    auto e = f->second;
    auto* o = e.o;
    auto& bk = *e.bk;
    auto lv = bk.find(e.p);
    if (lv == bk.end()) return false;

    lv->second.q.erase(e.it);
    lv->second.qty = lv->second.qty >= o->q ? lv->second.qty - o->q : 0;
    if (lv->second.q.empty()) bk.erase(lv);
    pool.free(o);
    ref.erase(f);
    return true;
}

bool Book::mod(uint64_t id, double np, uint64_t nq) {
    auto f = ref.find(id);
    if (f == ref.end()) return false;
    auto e = f->second;
    auto* o = e.o;
    if (np != o->p) {
        Order u = *o; u.p = np; u.q = nq;
        if (!cancel(id)) return false;
        add(u);
        return true;
    } else {
        auto& bk = *e.bk;
        auto lv = bk.find(e.p);
        if (lv == bk.end()) return false;
        lv->second.qty = lv->second.qty >= o->q ? lv->second.qty - o->q : 0;
        lv->second.qty += nq;
        o->q = nq;
        return true;
    }
}

void Book::match() {
    while (!bids.empty() && !asks.empty()) {
        auto bi = bids.begin(), ai = asks.begin();
        double bp = bi->first, ap = ai->first;
        if (bp < ap) break;

        auto* bo = bi->second.q.front();
        auto* ao = ai->second.q.front();
        uint64_t mq = min(bo->q, ao->q);
        double mp = ao->p;

        Trade t{bo->id, ao->id, mp, mq, now_ns()};
        ts.push_back(t);

        bo->q -= mq;
        ao->q -= mq;
        bi->second.qty -= mq;
        ai->second.qty -= mq;

        if (!bo->q) {
            bi->second.q.pop_front();
            ref.erase(bo->id);
            pool.free(bo);
            if (bi->second.q.empty()) bids.erase(bi);
        }
        if (!ao->q) {
            ai->second.q.pop_front();
            ref.erase(ao->id);
            pool.free(ao);
            if (ai->second.q.empty()) asks.erase(ai);
        }
    }
}

void Book::snap(size_t d, vector<LvlP>& bs, vector<LvlP>& as) const {
    bs.clear(); as.clear();
    size_t c = 0;
    for (auto it = bids.begin(); it != bids.end() && c < d; it++, c++) bs.emplace_back(it->first, it->second.qty);
    c = 0;
    for (auto it = asks.begin(); it != asks.end() && c < d; it++, c++) as.emplace_back(it->first, it->second.qty);
}

void Book::print(size_t d) const {
    vector<LvlP> bs, as;
    snap(d, bs, as);
    cout << "\nBOOK\nBIDS               ASKS\nPrc     Qty       Prc     Qty\n";
    cout << "------  ---       ------  ---\n";
    size_t m = max(bs.size(), as.size());
    for (size_t i = 0; i < m; i++) {
        if (i < bs.size()) cout << bs[i].p << "   " << bs[i].q;
        else cout << "            ";
        cout << "       ";
        if (i < as.size()) cout << as[i].p << "   " << as[i].q;
        cout << "\n";
    }
    cout << "\n";
}
