#include <iostream>
#include <unordered_map>
#include <list>
#include <queue>
#include <chrono>
#include <mutex>
#include <optional>
#include <thread>
#include <atomic>
#include <string>
#include <cassert>

// A hybrid TTL + LRU cache implementation

template<typename Key, typename Value>
class HybridCache {
private:
    struct CacheEntry {
        Value value;
        std::chrono::steady_clock::time_point expiry;
        typename std::list<Key>::iterator lruIt;
    };

    std::unordered_map<Key, CacheEntry> store;
    std::list<Key> lruList; // Most recently used at the back

    using ExpiryEntry = std::pair<std::chrono::steady_clock::time_point, Key>;
    struct CompareExpiry {
        bool operator()(const ExpiryEntry& a, const ExpiryEntry& b) const {
            return a.first > b.first;
        }
    };
    std::priority_queue<ExpiryEntry, std::vector<ExpiryEntry>, CompareExpiry> expiryQueue;

    std::mutex mtx;
    size_t capacity;

    std::atomic<bool> stopBackgroundThread;
    std::thread cleanerThread;

    void cleanupExpired() {
        auto now = std::chrono::steady_clock::now();
        while (!expiryQueue.empty() && expiryQueue.top().first <= now) {
            Key key = expiryQueue.top().second;
            expiryQueue.pop();
            auto it = store.find(key);
            if (it != store.end() && it->second.expiry <= now) {
                lruList.erase(it->second.lruIt);
                store.erase(it);
            }
        }
    }

    void backgroundCleanupLoop() {
        while (!stopBackgroundThread.load()) {
            {
                std::lock_guard<std::mutex> lock(mtx);
                cleanupExpired();
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

public:
    HybridCache(size_t cap) : capacity(cap), stopBackgroundThread(false) {
        cleanerThread = std::thread([this]() { backgroundCleanupLoop(); });
    }

    ~HybridCache() {
        stopBackgroundThread.store(true);
        if (cleanerThread.joinable()) {
            cleanerThread.join();
        }
    }

    void put(const Key& key, const Value& value, int ttlSeconds) {
        std::lock_guard<std::mutex> lock(mtx);
        auto now = std::chrono::steady_clock::now();
        auto expiry = now + std::chrono::seconds(ttlSeconds);

        if (store.find(key) != store.end()) {
            lruList.erase(store[key].lruIt);
        } else if (store.size() >= capacity) {
            Key lruKey = lruList.front();
            lruList.pop_front();
            store.erase(lruKey);
        }

        lruList.push_back(key);
        store[key] = {value, expiry, std::prev(lruList.end())};
        expiryQueue.push({expiry, key});
    }

    std::optional<Value> get(const Key& key) {
        std::lock_guard<std::mutex> lock(mtx);
        auto now = std::chrono::steady_clock::now();

        auto it = store.find(key);
        if (it == store.end() || it->second.expiry <= now)
            return std::nullopt;

        lruList.erase(it->second.lruIt);
        lruList.push_back(key);
        it->second.lruIt = std::prev(lruList.end());

        return it->second.value;
    }

    bool exists(const Key& key) {
        std::lock_guard<std::mutex> lock(mtx);
        auto now = std::chrono::steady_clock::now();
        auto it = store.find(key);
        return it != store.end() && it->second.expiry > now;
    }

    bool remove(const Key& key) {
        std::lock_guard<std::mutex> lock(mtx);
        auto it = store.find(key);
        if (it == store.end()) return false;
        lruList.erase(it->second.lruIt);
        store.erase(it);
        return true;
    }

    size_t size() {
        std::lock_guard<std::mutex> lock(mtx);
        return store.size();
    }
};

// ---------------------- TEST DRIVER ----------------------
int main() {
    HybridCache<std::string, std::string> cache(5);

    cache.put("a", "apple", 3); // expires in 3 seconds
    cache.put("b", "banana", 5);
    cache.put("c", "cherry", 6);

    std::cout << "Initial size: " << cache.size() << "\n";
    assert(cache.exists("a"));
    assert(cache.exists("b"));
    assert(cache.exists("c"));

    std::this_thread::sleep_for(std::chrono::seconds(4));

    std::cout << "After 4 seconds...\n";
    std::cout << "Exists a: " << cache.exists("a") << "\n"; // should be false
    std::cout << "Exists b: " << cache.exists("b") << "\n"; // true
    std::cout << "Exists c: " << cache.exists("c") << "\n"; // true

    std::cout << "Final size: " << cache.size() << "\n";

    return 0;
}