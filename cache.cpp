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

    bool access(int address, int level, bool promote = false) {
        cout << "\nCache Name: | L" << level << " Cache |" << endl;
        if (policy == Policy::LRU) {
            if (lru_map.find(address) != lru_map.end()) {
                if (!promote) {
                    lru_cache.erase(lru_map[address]);
                    lru_cache.push_front(address);
                    lru_map[address] = lru_cache.begin();
                }
                hits++;
                cout << "Hit - Address " << address << " found in L" << level << " cache." << endl;
                return true;
            }
        } else if (policy == Policy::FIFO) {
            queue<int> tempQueue = fifo_cache;
            while (!tempQueue.empty()) {
                if (tempQueue.front() == address) {
                    hits++;
                    cout << "Hit - Address " << address << " found in L" << level << " cache." << endl;
                    return true;
                }
                tempQueue.pop();
            }
        } else if (policy == Policy::LFU) {
            if (lfu_count.find(address) != lfu_count.end()) {
                lfu_count[address]++;
                hits++;
                cout << "Hit - Address " << address << " found in L" << level << " cache." << endl;
                return true;
            }
        }
        
        // MISS handling
        misses++;
        if (policy == Policy::LRU) {
            if (lru_cache.size() >= size) {
                int evicted = lru_cache.back();
                cout << "Evicting Block from L" << level << " - " << evicted << endl;
                dirtyBit.erase(evicted);
                lru_map.erase(evicted);
                lru_cache.pop_back();
            }
            cout << "Adding Block to L" << level << " - " << address << endl;
            lru_cache.push_front(address);
            lru_map[address] = lru_cache.begin();
        } else if (policy == Policy::FIFO) {
            if (fifo_cache.size() >= size) {
                int evicted = fifo_cache.front();
                cout << "Evicting Block from L" << level << " - " << evicted << endl;
                fifo_cache.pop();
            }
            cout << "Adding Block to L" << level << " - " << address << endl;
            fifo_cache.push(address);
        } else if (policy == Policy::LFU) {
            if (lfu_count.size() >= size) {
                int leastFreq = INT_MAX, evicted = -1;
                for (auto& entry : lfu_count) {
                    if (entry.second < leastFreq) {
                        leastFreq = entry.second;
                        evicted = entry.first;
                    }
                }
                if (evicted != -1) {
                    cout << "Evicting Block from L" << level << " - " << evicted << endl;
                    lfu_count.erase(evicted);
                }
            }
            cout << "Adding Block to L" << level << " - " << address << endl;
            lfu_count[address] = 1;
        }
        return false;
    }

    void remove(int address) {
        lru_map.erase(address);
        fifo_cache = queue<int>();
        lfu_count.erase(address);
    }

    void visualize(int level) {
        cout << "Final State: |L" << level << "| Cache Blocks: [ ";
        if (policy == Policy::LRU) {
            for (int addr : lru_cache) cout << addr << " ";
        } else if (policy == Policy::FIFO) {
            queue<int> temp = fifo_cache;
            while (!temp.empty()) {
                cout << temp.front() << " ";
                temp.pop();
            }
        } else if (policy == Policy::LFU) {
            for (auto& pair : lfu_count) cout << pair.first << " ";
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
        levels.emplace_back(l2_size, Policy::FIFO, WritePolicy::WRITE_THROUGH);
        levels.emplace_back(l3_size, Policy::LFU, WritePolicy::WRITE_BACK);
    }

    void access(int address) {
        for (size_t i = 0; i < levels.size(); i++) {
            if (levels[i].access(address, i + 1)) {
                if (i > 0) {
                    cout << "PROMOTION: Moving Address " << address << " from L" << (i + 1) << " to L1\n";
                    levels[i].remove(address);
                    levels[0].access(address, 1, true);
                }
                return;
            }
        }
        cout << "MISS: Fetching Address " << address << " from Main Memory and adding to L1.\n";
        mainMemory[address] = address;
        levels[0].access(address, 1, true);
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
        int addr = rand() % 15;
        cout << "\nAccessing Address: " << addr << "\n";
        mlc.access(addr);
        mlc.visualize();
    }
    
    mlc.showStats();
    return 0;
}
