// multilevel_cache_policies.cpp
#include <bits/stdc++.h>
using namespace std;

// ---------- LRU (for L1) ----------
struct LRU {
    int cap;
    int level;
    list<int> q; // front = MRU
    unordered_map<int, list<int>::iterator> pos;
    int hits=0, misses=0;

    LRU(int c=4,int lvl=1):cap(c),level(lvl){}

    bool containsMoveToFront(int addr) {
        auto it = pos.find(addr);
        if (it==pos.end()) { misses++; return false; }
        q.erase(it->second);
        q.push_front(addr);
        pos[addr] = q.begin();
        hits++;
        return true;
    }

    void insert(int addr) {
        auto it = pos.find(addr);
        if (it != pos.end()) { // already present -> move
            q.erase(it->second);
            q.push_front(addr);
            pos[addr] = q.begin();
            return;
        }
        if ((int)q.size() >= cap) {
            int ev = q.back(); q.pop_back(); pos.erase(ev);
            cout << "Evicting " << ev << " from L" << level << "\n";
        }
        q.push_front(addr);
        pos[addr] = q.begin();
        cout << "Inserted " << addr << " into L" << level << "\n";
    }

    void remove(int addr) {
        auto it = pos.find(addr);
        if (it != pos.end()) { q.erase(it->second); pos.erase(it); }
    }

    void visualize() const {
        cout << "L" << level << " [MRU..LRU]: ";
        for (int x:q) cout<<x<<" ";
        cout<<"\n";
    }
};

// ---------- FIFO (for L2) ----------
struct FIFO {
    int cap;
    int level;
    queue<int> q;
    unordered_set<int> present; // O(1) membership
    int hits=0, misses=0;

    FIFO(int c=4,int lvl=2):cap(c),level(lvl){}

    bool contains(int addr) {
        if (present.find(addr)==present.end()) { misses++; return false; }
        hits++;
        return true;
    }

    void insert(int addr) {
        if (present.find(addr) != present.end()) return; // already present
        if ((int)q.size() >= cap) {
            int ev = q.front(); q.pop(); present.erase(ev);
            cout << "Evicting " << ev << " from L" << level << "\n";
        }
        q.push(addr); present.insert(addr);
        cout << "Inserted " << addr << " into L" << level << "\n";
    }

    void remove(int addr) {
        if (present.find(addr) == present.end()) return;
        // rebuild queue excluding addr (O(n) but fine for simple simulator)
        queue<int> nq;
        while (!q.empty()) {
            int v = q.front(); q.pop();
            if (v != addr) nq.push(v);
        }
        q = move(nq);
        present.erase(addr);
    }

    void visualize() const {
        cout << "L" << level << " [FIFO order front..back]: ";
        queue<int> tmp = q;
        while (!tmp.empty()) { cout<<tmp.front()<<" "; tmp.pop(); }
        cout<<"\n";
    }
};

// ---------- LFU (for L3) ----------
struct LFU {
    int cap;
    int level;
    unordered_map<int,int> freq;
    unordered_map<int,long long> timeStamp; // tie-breaker: lower = older
    long long timer = 0;
    int hits=0, misses=0;

    LFU(int c=4,int lvl=3):cap(c),level(lvl){}

    bool containsInc(int addr) {
        auto it = freq.find(addr);
        if (it==freq.end()) { misses++; return false; }
        it->second++;
        timeStamp[addr] = ++timer;
        hits++;
        return true;
    }

    void insert(int addr) {
        if (freq.find(addr) != freq.end()) { // already present -> increment and update time
            freq[addr]++; timeStamp[addr] = ++timer; return;
        }
        if ((int)freq.size() >= cap) {
            // evict least freq, break ties by oldest timeStamp
            int evicted = -1;
            int leastF = INT_MAX;
            long long oldest = LLONG_MAX;
            for (auto &p : freq) {
                int a = p.first; int f = p.second;
                long long t = timeStamp[a];
                if (f < leastF || (f == leastF && t < oldest)) {
                    leastF = f; oldest = t; evicted = a;
                }
            }
            if (evicted != -1) {
                freq.erase(evicted);
                timeStamp.erase(evicted);
                cout << "Evicting " << evicted << " from L" << level << "\n";
            }
        }
        freq[addr] = 1;
        timeStamp[addr] = ++timer;
        cout << "Inserted " << addr << " into L" << level << "\n";
    }

    void remove(int addr) {
        freq.erase(addr);
        timeStamp.erase(addr);
    }

    void visualize() const {
        cout << "L" << level << " [addr:freq]: ";
        for (auto &p: freq) cout << p.first << ":" << p.second << " ";
        cout << "\n";
    }
};

// ---------- MultiLevel controller ----------
struct MultiLevel {
    LRU L1;
    FIFO L2;
    LFU L3;
    int memoryFetches = 0;

    MultiLevel(int c1,int c2,int c3): L1(c1,1), L2(c2,2), L3(c3,3) {}

    void access(int addr) {
        cout << "\nAccess " << addr << ":\n";
        if (L1.containsMoveToFront(addr)) { cout << "Hit L1\n"; return; }

        if (L2.contains(addr)) {
            cout << "Hit L2 -> promote to L1\n";
            L2.remove(addr);
            L1.insert(addr);
            return;
        }

        if (L3.containsInc(addr)) {
            cout << "Hit L3 -> promote to L1\n";
            L3.remove(addr);
            L1.insert(addr);
            return;
        }

        // Miss in all -> fetch from memory and insert into L1
        cout << "Miss in L1,L2,L3 -> fetch from memory -> insert into L1\n";
        memoryFetches++;
        L1.insert(addr);
    }

    void visualize() const {
        L1.visualize();
        L2.visualize();
        L3.visualize();
    }

    void stats() const {
        cout << "\n=== Stats ===\n";
        cout << "L1: hits="<<L1.hits<<", misses="<<L1.misses<<"\n";
        cout << "L2: hits="<<L2.hits<<", misses="<<L2.misses<<"\n";
        cout << "L3: hits="<<L3.hits<<", misses="<<L3.misses<<"\n";
        cout << "Memory fetches: " << memoryFetches << "\n";
    }
};

// ---------- main (demo) ----------
int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int c1,c2,c3,n;
    cout << "Enter sizes for L1(LRU) L2(FIFO) L3(LFU): ";
    if (!(cin>>c1>>c2>>c3)) return 0;
    cout << "Enter number of accesses: ";
    cin >> n;

    MultiLevel ml(c1,c2,c3);

    cout << "Enter 0 to use random sequence, 1 to input addresses: ";
    int mode; cin>>mode;
    vector<int> seq;
    if (mode==0) {
        srand((unsigned)time(nullptr));
        for (int i=0;i<n;++i) seq.push_back(rand()%10);
    } else {
        cout << "Enter " << n << " addresses:\n";
        for (int i=0;i<n;++i){ int a; cin>>a; seq.push_back(a); }
    }

    for (int a: seq) {
        ml.access(a);
        ml.visualize();
    }
    ml.stats();
   
    return 0;
}

