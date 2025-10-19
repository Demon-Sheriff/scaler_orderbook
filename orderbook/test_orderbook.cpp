#include "order_book.h"
#include <iostream>
#include <iomanip>
using namespace std;

void test_book() {
    cout << "=== Order Book Test ===\n";
    Book bk;

    cout << "\n> Adding buys\n";
    bk.add({1001, true, 100.5, 1000, now_ns()});
    bk.add({1002, true, 100.3, 400, now_ns()});
    bk.add({1003, true, 99.8, 600, now_ns()});

    cout << "> Adding sells\n";
    bk.add({2001, false, 100.7, 800, now_ns()});
    bk.add({2002, false, 101.0, 300, now_ns()});
    bk.add({2003, false, 100.75, 150, now_ns()});

    bk.print(5);

    cout << "\n> Cancel order #1002: " << (bk.cancel(1002) ? "OK" : "FAIL") << "\n";
    bk.print(5);

    cout << "\n> Amend order #1003 qty 600->900: "
         << (bk.mod(1003, 99.8, 900) ? "OK" : "FAIL") << "\n";
    bk.print(5);

    cout << "\n> Amend order #2001 price 100.7->100.4: "
         << (bk.mod(2001, 100.4, 800) ? "OK" : "FAIL") << "\n";
    bk.print(5);

    if (!bk.get_ts().empty()) {
        cout << "\n> Trades executed:\n";
        for (auto& t : bk.get_ts())
            cout << "Buy#" << t.b << " x Sell#" << t.s
                 << " | " << t.q << " @ " << fixed << setprecision(2) << t.p << "\n";
    }

    cout << "\n> Matching test\n";
    Book m;
    m.add({3001, true, 100.5, 1000, now_ns()});
    m.add({3002, false, 100.25, 600, now_ns()});
    m.add({3003, false, 100.4, 500, now_ns()});

    cout << "\nTrades:\n";
    for (auto& t : m.get_ts())
        cout << "TRADE: " << t.q << " @ " << fixed << setprecision(2) << t.p
             << " (B" << t.b << " x S" << t.s << ")\n";

    cout << "\n> Final book snapshot:\n";
    m.print(3);

    cout << "\n=== Done ===\n";
}

int main() {
    ios::sync_with_stdio(false);
    test_book();
    return 0;
}
