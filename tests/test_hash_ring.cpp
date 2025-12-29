#include <iostream>
#include <unordered_map>
#include <iomanip>
#include "../src/sharding/hash_ring.h"

using namespace kvstore;

void PrintDistribution(const HashRing& ring, int num_keys) {
    std::unordered_map<std::string, int> distribution;
    
    // Initialize counters
    auto shards = ring.GetAllShards();
    for (const auto& shard : shards) {
        distribution[shard.shard_id] = 0;
    }
    
    // Distribute keys
    for (int i = 0; i < num_keys; ++i) {
        std::string key = "key_" + std::to_string(i);
        std::string shard = ring.GetShardForKey(key);
        distribution[shard]++;
    }
    
    // Print distribution
    std::cout << "\nKey Distribution (total: " << num_keys << " keys):" << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    
    for (const auto& [shard_id, count] : distribution) {
        double percentage = (100.0 * count) / num_keys;
        std::cout << std::setw(10) << shard_id << ": " 
                  << std::setw(6) << count << " keys (" 
                  << std::fixed << std::setprecision(2) << percentage << "%)" 
                  << std::endl;
    }
    std::cout << "----------------------------------------" << std::endl;
}

int main() {
    std::cout << "==================================" << std::endl;
    std::cout << "Hash Ring Test" << std::endl;
    std::cout << "==================================" << std::endl;
    
    // Create hash ring with 150 virtual nodes per shard
    HashRing ring(150);
    
    std::cout << "\n[Test 1] Adding 3 shards..." << std::endl;
    ring.AddShard("shard-1", "localhost:50051");
    ring.AddShard("shard-2", "localhost:50052");
    ring.AddShard("shard-3", "localhost:50053");
    
    std::cout << "\nTotal shards: " << ring.GetShardCount() << std::endl;
    
    // Test key distribution with 3 shards
    PrintDistribution(ring, 10000);
    
    std::cout << "\n[Test 2] Looking up specific keys..." << std::endl;
    std::vector<std::string> test_keys = {"user:1", "user:2", "user:3", "order:100", "product:50"};
    for (const auto& key : test_keys) {
        std::string shard = ring.GetShardForKey(key);
        std::cout << "  Key '" << key << "' -> " << shard << std::endl;
    }
    
    std::cout << "\n[Test 3] Adding 4th shard..." << std::endl;
    ring.AddShard("shard-4", "localhost:50054");
    
    // Check how distribution changes
    PrintDistribution(ring, 10000);
    
    std::cout << "\n[Test 4] Removing shard-2..." << std::endl;
    ring.RemoveShard("shard-2");
    
    PrintDistribution(ring, 10000);
    
    std::cout << "\n[Test 5] Looking up keys after removal..." << std::endl;
    for (const auto& key : test_keys) {
        std::string shard = ring.GetShardForKey(key);
        std::cout << "  Key '" << key << "' -> " << shard << std::endl;
    }
    
    std::cout << "\n[Test 6] Getting shard information..." << std::endl;
    auto all_shards = ring.GetAllShards();
    std::cout << "Current shards in ring:" << std::endl;
    for (const auto& shard : all_shards) {
        std::cout << "  - " << shard.shard_id << " @ " << shard.address 
                  << (shard.is_available ? " (available)" : " (unavailable)") << std::endl;
    }
    
    std::cout << "\n==================================" << std::endl;
    std::cout << "All tests completed!" << std::endl;
    std::cout << "==================================" << std::endl;
    
    return 0;
}
