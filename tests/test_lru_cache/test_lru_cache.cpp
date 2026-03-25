#include "lru_cache/lru_cache.h"

#include <gtest/gtest.h>

#include <string>
#include <utility>

TEST(LRUCacheTest, PutAndGetValue) {
    bmngxn::lru_cache<int, std::string> cache(3);

    cache.put(1, "one");
    cache.put(2, "two");

    const auto value = cache.get(1);

    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(*value, "one");
    EXPECT_EQ(cache.size(), 2);
}

TEST(LRUCacheTest, GetMissingKeyReturnsNullopt) {
    bmngxn::lru_cache<int, std::string> cache(2);

    EXPECT_FALSE(cache.get(99).has_value());
}

TEST(LRUCacheTest, ExistingKeyIsUpdated) {
    bmngxn::lru_cache<int, std::string> cache(2);

    cache.put(1, "one");
    cache.put(1, "sus");

    const auto value = cache.get(1);

    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(*value, "sus");
    EXPECT_EQ(cache.size(), 1);
}

TEST(LRUCacheTest, LeastRecentlyUsedEntryIsEvicted) {
    bmngxn::lru_cache<int, std::string> cache(2);

    cache.put(1, "one");
    cache.put(2, "two");
    ASSERT_TRUE(cache.get(1).has_value());

    cache.put(3, "three");

    EXPECT_TRUE(cache.contains(1));
    EXPECT_FALSE(cache.contains(2));
    EXPECT_TRUE(cache.contains(3));
}

TEST(LRUCacheTest, GetPromotesEntryToMostRecentlyUsed) {
    bmngxn::lru_cache<int, std::string> cache(2);

    cache.put(1, "one");
    cache.put(2, "two");
    ASSERT_TRUE(cache.get(1).has_value());

    cache.put(3, "three");

    EXPECT_TRUE(cache.contains(1));
    EXPECT_FALSE(cache.contains(2));
}

TEST(LRUCacheTest, EraseRemovesExistingKey) {
    bmngxn::lru_cache<int, std::string> cache(2);

    cache.put(1, "one");
    cache.put(2, "two");

    EXPECT_TRUE(cache.erase(1));
    EXPECT_FALSE(cache.contains(1));
    EXPECT_EQ(cache.size(), 1);
}

TEST(LRUCacheTest, EraseMissingKeyReturnsFalse) {
    bmngxn::lru_cache<int, std::string> cache(2);

    cache.put(1, "one");

    EXPECT_FALSE(cache.erase(2));
    EXPECT_EQ(cache.size(), 1);
}

TEST(LRUCacheTest, ZeroCapacityCacheStoresNothing) {
    bmngxn::lru_cache<int, std::string> cache(0);

    cache.put(1, "one");

    EXPECT_EQ(cache.size(), 0);
    EXPECT_FALSE(cache.contains(1));
    EXPECT_FALSE(cache.get(1).has_value());
}

TEST(LRUCacheTest, CopyConstructorCreatesIndependentCache) {
    bmngxn::lru_cache<int, std::string> original(2);
    original.put(1, "one");
    original.put(2, "two");

    bmngxn::lru_cache<int, std::string> copy(original);
    copy.put(3, "three");

    EXPECT_TRUE(original.contains(1));
    EXPECT_TRUE(original.contains(2));
    EXPECT_FALSE(original.contains(3));

    EXPECT_FALSE(copy.contains(1));
    EXPECT_TRUE(copy.contains(2));
    EXPECT_TRUE(copy.contains(3));
}

TEST(LRUCacheTest, MoveConstructorTransfersState) {
    bmngxn::lru_cache<int, std::string> source(2);
    source.put(1, "one");
    source.put(2, "two");

    bmngxn::lru_cache<int, std::string> moved(std::move(source));

    EXPECT_EQ(moved.capacity(), 2);
    EXPECT_TRUE(moved.contains(1));
    EXPECT_TRUE(moved.contains(2));
    EXPECT_EQ(moved.size(), 2);
}
