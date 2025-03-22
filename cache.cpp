#include <iostream>
#include <vector>
#include <unordered_map>
#include <list>
#include <queue>
#include <cstdlib>
#include <ctime>
#include <climits>

using namespace std;

enum class Policy { LRU, FIFO, LFU };
enum class WritePolicy { WRITE_BACK, WRITE_THROUGH };

unordered_map<int, int> mainMemory; // Simulating Main Memory

class Cache {
private:
    int size;
    Policy policy;
    WritePolicy writePolicy;
    list<int> lru_cache;
    unordered_map<int, list<int>::iterator> lru_map;
    queue<int> fifo_cache;
    unordered_map<int, int> lfu_count;
    unordered_map<int, bool> dirtyBit;
    int hits, misses;

public:
    Cache(int s, Policy p, WritePolicy wp) : size(s), policy(p), writePolicy(wp), hits(0), misses(0) {}

    bool access(int address, int level) {
        cout << "\nCache Name: | L" << level << " Cache |" << endl;
        if (policy == Policy::LRU) {
            if (lru_map.find(address) != lru_map.end()) {
                lru_cache.erase(lru_map[address]);
                lru_cache.push_front(address);
                lru_map[address] = lru_cache.begin();
                hits++;
                cout << "Hit - Address " << address << " found in L" << level << " cache." << endl;
                return true;
            } else {
                if (lru_cache.size() >= size) {
                    int evicted = lru_cache.back();
                    cout << "Evicting Block from L" << level << " - " << evicted << endl;
                    if (writePolicy == WritePolicy::WRITE_BACK && dirtyBit[evicted]) {
                        cout << "Writing back dirty block " << evicted << " to main memory." << endl;
                        mainMemory[evicted] = evicted; // Simulating memory write
                    }
                    dirtyBit.erase(evicted);
                    lru_map.erase(evicted);
                    lru_cache.pop_back();
                }
                cout << "Adding Block to L" << level << " - " << address << endl;
                lru_cache.push_front(address);
                lru_map[address] = lru_cache.begin();
                if (writePolicy == WritePolicy::WRITE_THROUGH) {
                    cout << "Writing data for address " << address << " directly to memory." << endl;
                    mainMemory[address] = address; // Immediate write
                } else {
                    dirtyBit[address] = true;
                }
                misses++;
                return false;
            }
        }
        return false;
    }

    void visualize(int level) {
        cout << "Final State: |L" << level << "| Cache Blocks: [ ";
        for (int addr : lru_cache) {
            cout << addr << " ";
        }
        cout << "]\n";
    }
    
    void showStats(int level) {
        cout << "Cache L" << level << " - Hits: " << hits << " | Misses: " << misses << "\n";
        cout << "Hit Ratio: " << (hits * 100.0 / (hits + misses)) << "%\n";
    }
};

class MultiLevelCache {
private:
    vector<Cache> levels;

public:
    MultiLevelCache(int l1_size, int l2_size, int l3_size) {
        levels.emplace_back(l1_size, Policy::LRU, WritePolicy::WRITE_BACK);
        levels.emplace_back(l2_size, Policy::LRU, WritePolicy::WRITE_THROUGH);
        levels.emplace_back(l3_size, Policy::LRU, WritePolicy::WRITE_BACK);
    }

    void access(int address) {
        for (size_t i = 0; i < levels.size(); i++) {
            if (levels[i].access(address, i + 1)) return;
        }
        cout << "Fetching Address " << address << " from Main Memory." << endl;
        mainMemory[address] = address; // Load into memory if not present
    }

    void visualize() {
        for (size_t i = 0; i < levels.size(); i++) {
            levels[i].visualize(i + 1);
        }
    }

    void showStats() {
        for (size_t i = 0; i < levels.size(); i++) {
            levels[i].showStats(i + 1);
        }
    }
};

int main() {
    int l1_size, l2_size, l3_size, num_accesses;
    cout << "Enter L1 Cache Size: ";
    cin >> l1_size;
    cout << "Enter L2 Cache Size: ";
    cin >> l2_size;
    cout << "Enter L3 Cache Size: ";
    cin >> l3_size;
    cout << "Enter Number of Memory Accesses: ";
    cin >> num_accesses;
    
    MultiLevelCache mlc(l1_size, l2_size, l3_size);
    srand(time(0));
    
    for (int i = 0; i < num_accesses; i++) {
        int addr = rand() % 100;
        cout << "\nAccessing Address: " << addr << "\n";
        mlc.access(addr);
        mlc.visualize();
    }
    
    mlc.showStats();
    return 0;
}
