#include "lru_cache/concurrent_lru_cache.h"

#include <gtest/gtest.h>

#include <atomic>
#include <string>
#include <thread>
#include <vector>

class ConcurrentLRUCacheTest : public ::testing::Test {};

TEST_F(ConcurrentLRUCacheTest, PutAndGetValue) {
    bmngxn::concurrent_lru_cache<int, std::string> cache(16);

    cache.put(1, "one");
    cache.put(2, "two");

    const auto value = cache.get(1);

    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(*value, "one");
}

TEST_F(ConcurrentLRUCacheTest, MissingKeyReturnsNullopt) {
    bmngxn::concurrent_lru_cache<int, std::string> cache(16);

    EXPECT_FALSE(cache.get(99).has_value());
}

TEST_F(ConcurrentLRUCacheTest, EraseRemovesKey) {
    bmngxn::concurrent_lru_cache<int, std::string> cache(16);

    cache.put(1, "one");

    EXPECT_TRUE(cache.erase(1));
    EXPECT_FALSE(cache.get(1).has_value());
}

TEST_F(ConcurrentLRUCacheTest, ConcurrentPutsAndGetsDoNotLoseInsertedKeys) {
    bmngxn::concurrent_lru_cache<int, int, 8> cache(128);

    constexpr int kThreadCount = 8;
    constexpr int kKeysPerThread = 8;

    std::vector<std::thread> threads;
    threads.reserve(kThreadCount);

    for (int t = 0; t < kThreadCount; ++t) {
        threads.emplace_back([&cache, t]() {
            for (int i = 0; i < kKeysPerThread; ++i) {
                const int key = t * 100 + i;
                cache.put(key, key * 10);
                const auto value = cache.get(key);
                ASSERT_TRUE(value.has_value());
                EXPECT_EQ(*value, key * 10);
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    for (int t = 0; t < kThreadCount; ++t) {
        for (int i = 0; i < kKeysPerThread; ++i) {
            const int key = t * 100 + i;
            const auto value = cache.get(key);
            ASSERT_TRUE(value.has_value());
            EXPECT_EQ(*value, key * 10);
        }
    }
}

TEST_F(ConcurrentLRUCacheTest, ConcurrentReadsOfSameKeyAreSafe) {
    bmngxn::concurrent_lru_cache<int, int, 4> cache(32);
    cache.put(7, 77);

    constexpr int kThreadCount = 8;
    constexpr int kReadsPerThread = 1000;

    std::atomic<int> successful_reads = 0;
    std::vector<std::thread> threads;
    threads.reserve(kThreadCount);

    for (int t = 0; t < kThreadCount; ++t) {
        threads.emplace_back([&cache, &successful_reads]() {
            for (int i = 0; i < kReadsPerThread; ++i) {
                const auto value = cache.get(7);
                ASSERT_TRUE(value.has_value());
                EXPECT_EQ(*value, 77);
                successful_reads.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(successful_reads.load(std::memory_order_relaxed), kThreadCount * kReadsPerThread);
}
