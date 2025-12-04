#include <mutex>
#include <priority_queue>
#include <map>
#include <chrono>
// Implement a simple in-memory key-value store with TTL (time-to-live) functionality.
// The store should support the following operations:
// put(key, value, ttl_in_seconds)
// get(key) → returns value or null
template<typename T, typename U>
class KeyValueStore {
private:
    struct ValueEntry { 
        U value; 
        std::chrono::steady_clock::time_point expiry; 
    };
    
    std::unordered_map<T, ValueEntry> store;

    std::priority_queue<std::pair<std::chrono::steady_clock::time_point,  T>,
                        std::vector<std::pair<std::chrono::steady_clock::time_point,  T>>,
                        std::greater<>> expiry_queue;

    std::mutex mtx;

public:
    void put(T key, U value, int ttl_in_seconds) {
        std::lock_guard<std::mutex> lock(mtx);
        auto expiry_time = std::chrono::steady_clock::now() + std::chrono::seconds(ttl_in_seconds);
        store[key] = {value, expiry_time};
        expiry_queue.push({expiry_time, key});
    }
    U get(T key) {
        std::lock_guard<std::mutex> lock(mtx);
        auto now = std::chrono::steady_clock::now();
        
        // Clean up expired entries
        while (!expiry_queue.empty() && expiry_queue.top().first <= now) {
            T expired_key = expiry_queue.top().second;
            expiry_queue.pop();
            auto it = store.find(expired_key);
            if (it != store.end() && it->second.expiry <= now) {
                store.erase(it);
            }
        }
        
        auto it = store.find(key);
        if (it != store.end()) {
            return it->second.value;
        }
        return U(); // return default value if key not found
    }
};
