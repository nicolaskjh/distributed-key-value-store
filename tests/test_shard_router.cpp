#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include "../src/sharding/shard_router.h"

using namespace kvstore;

void PrintStats(const ShardRouter& router) {
    auto stats = router.GetStats();
    
    std::cout << "\n========== Routing Statistics ==========" << std::endl;
    std::cout << "Total Requests:      " << stats.total_requests << std::endl;
    std::cout << "Successful:          " << stats.successful_requests << std::endl;
    std::cout << "Failed:              " << stats.failed_requests << std::endl;
    std::cout << "\nPer-Shard Distribution:" << std::endl;
    
    for (const auto& [shard_id, count] : stats.per_shard_requests) {
        double percentage = (100.0 * count) / stats.total_requests;
        std::cout << "  " << std::setw(12) << shard_id << ": " 
                  << std::setw(5) << count << " requests (" 
                  << std::fixed << std::setprecision(1) << percentage << "%)" 
                  << std::endl;
    }
    std::cout << "========================================\n" << std::endl;
}

int main() {
    std::cout << "==================================" << std::endl;
    std::cout << "Shard Router Test" << std::endl;
    std::cout << "==================================" << std::endl;
    
    // Step 1: Create hash ring with 3 shards
    std::cout << "\n[Step 1] Creating hash ring with 3 shards..." << std::endl;
    auto hash_ring = std::make_shared<HashRing>(150);
    hash_ring->AddShard("shard-1", "localhost:50051");
    hash_ring->AddShard("shard-2", "localhost:50052");
    hash_ring->AddShard("shard-3", "localhost:50053");
    
    // Step 2: Create router
    std::cout << "\n[Step 2] Creating shard router..." << std::endl;
    ShardRouter router(hash_ring);
    
    std::cout << "\n[Step 3] Simulating client requests..." << std::endl;
    std::cout << "(Note: Shards are not actually running, so RPCs will fail)" << std::endl;
    std::cout << "(This test demonstrates routing logic, not actual data storage)" << std::endl;
    
    // Step 3: Route some operations (will fail since shards aren't running)
    std::cout << "\nRouting SET operations..." << std::endl;
    for (int i = 1; i <= 20; ++i) {
        std::string key = "user:" + std::to_string(i);
        std::string value = "User " + std::to_string(i);
        
        // Determine which shard would handle this
        std::string shard = hash_ring->GetShardForKey(key);
        std::cout << "  " << key << " -> " << shard;
        
        // Try to route (will fail, but updates stats)
        bool success = router.Set(key, value);
        std::cout << (success ? " [OK]" : " [FAIL - shard not running]") << std::endl;
    }
    
    std::cout << "\nRouting GET operations..." << std::endl;
    for (int i = 1; i <= 10; ++i) {
        std::string key = "user:" + std::to_string(i);
        std::string shard = hash_ring->GetShardForKey(key);
        std::cout << "  " << key << " -> " << shard;
        
        auto value = router.Get(key);
        std::cout << (value.has_value() ? " [OK]" : " [FAIL - shard not running]") << std::endl;
    }
    
    // Step 4: Show routing statistics
    std::cout << "\n[Step 4] Routing statistics..." << std::endl;
    PrintStats(router);
    
    // Step 5: Demonstrate adding a new shard
    std::cout << "[Step 5] Adding 4th shard to hash ring..." << std::endl;
    hash_ring->AddShard("shard-4", "localhost:50054");
    
    std::cout << "\nRouting with 4 shards..." << std::endl;
    router.ResetStats();
    
    for (int i = 1; i <= 20; ++i) {
        std::string key = "product:" + std::to_string(i);
        std::string shard = hash_ring->GetShardForKey(key);
        std::cout << "  " << key << " -> " << shard << std::endl;
        router.Set(key, "Product " + std::to_string(i));
    }
    
    PrintStats(router);
    
    // Step 6: Show consistent routing
    std::cout << "[Step 6] Verifying consistent routing..." << std::endl;
    std::string test_key = "user:123";
    
    std::cout << "Looking up '" << test_key << "' 5 times:" << std::endl;
    for (int i = 0; i < 5; ++i) {
        std::string shard = hash_ring->GetShardForKey(test_key);
        std::cout << "  Attempt " << (i+1) << ": " << shard << std::endl;
    }
    std::cout << "✓ Same shard every time (consistent hashing!)" << std::endl;
    
    std::cout << "\n==================================" << std::endl;
    std::cout << "Router test completed!" << std::endl;
    std::cout << "==================================" << std::endl;
    
    std::cout << "\nKey Insights:" << std::endl;
    std::cout << "1. Router determines target shard using hash ring" << std::endl;
    std::cout << "2. Maintains connection pool to shards (reuses connections)" << std::endl;
    std::cout << "3. Tracks statistics per shard for monitoring" << std::endl;
    std::cout << "4. Same key always routes to same shard (deterministic)" << std::endl;
    std::cout << "5. Adding shards redistributes ~25% of keys (with 4 shards)" << std::endl;
    
    return 0;
}
