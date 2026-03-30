#include "hash_map/hash_map.h"

#include <gtest/gtest.h>
#include <string>
#include <unordered_map>

class HashMapTest : public ::testing::Test {
protected:
    bmngxn::hash_map<std::string, int> map;
};

TEST_F(HashMapTest, InsertAndGet) {
    map.insert("key1", 100);
    
    auto* value = map.get("key1");
    ASSERT_NE(value, nullptr);
    EXPECT_EQ(*value, 100);
}

TEST_F(HashMapTest, InsertUpdate) {
    map.insert("key1", 100);
    map.insert("key1", 200);
    
    auto* value = map.get("key1");
    ASSERT_NE(value, nullptr);
    EXPECT_EQ(*value, 200);
    EXPECT_EQ(map.get_size(), 1);
}

TEST_F(HashMapTest, GetNonexistent) {
    auto* value = map.get("nonexistent");
    EXPECT_EQ(value, nullptr);
}

TEST_F(HashMapTest, Contains) {
    map.insert("key1", 100);
    
    EXPECT_TRUE(map.contains("key1"));
    EXPECT_FALSE(map.contains("key2"));
}

TEST_F(HashMapTest, MultipleInsertions) {
    for (int i = 0; i < 10; ++i) {
        map.insert("key" + std::to_string(i), i * 10);
    }
    
    EXPECT_EQ(map.get_size(), 10);
    
    for (int i = 0; i < 10; ++i) {
        auto* value = map.get("key" + std::to_string(i));
        ASSERT_NE(value, nullptr);
        EXPECT_EQ(*value, i * 10);
    }
}

TEST_F(HashMapTest, Erase) {
    map.insert("key1", 100);
    map.insert("key2", 200);
    map.insert("key3", 300);
    
    EXPECT_EQ(map.get_size(), 3);
    
    EXPECT_TRUE(map.erase("key2"));
    EXPECT_EQ(map.get_size(), 2);
    EXPECT_FALSE(map.contains("key2"));
    EXPECT_EQ(map.get("key2"), nullptr);
}

TEST_F(HashMapTest, EraseNonexistent) {
    EXPECT_FALSE(map.erase("nonexistent"));
    EXPECT_EQ(map.get_size(), 0);
}

TEST_F(HashMapTest, EraseAndReinsert) {
    map.insert("key1", 100);
    EXPECT_TRUE(map.erase("key1"));
    
    map.insert("key1", 200);
    EXPECT_EQ(map.get_size(), 1);
    
    auto* value = map.get("key1");
    ASSERT_NE(value, nullptr);
    EXPECT_EQ(*value, 200);
}

// current cfg: cap = 16, MLF = 0.5 -> resize when size + 1 > 8
TEST_F(HashMapTest, Resize) {
    for (int i = 0; i < 8; ++i) {
        map.insert("key" + std::to_string(i), i);
    }
    
    std::size_t capacity_before = map.get_capacity();
    
    // trigger resize
    map.insert("key8", 8);
    
    std::size_t capacity_after = map.get_capacity();
    EXPECT_GT(capacity_after, capacity_before);
    
    EXPECT_EQ(map.get_size(), 9);
    for (int i = 0; i < 9; ++i) {
        auto* value = map.get("key" + std::to_string(i));
        ASSERT_NE(value, nullptr);
        EXPECT_EQ(*value, i);
    }
}

TEST_F(HashMapTest, Reserve) {
    std::size_t capacity_before = map.get_capacity();
    
    map.reserve(100);
    
    std::size_t capacity_after = map.get_capacity();
    EXPECT_GT(capacity_after, capacity_before);
}

TEST_F(HashMapTest, IntegerKeys) {
    bmngxn::hash_map<int, std::string> int_map;
    
    int_map.insert(42, "answer");
    int_map.insert(100, "hundred");
    int_map.insert(999, "big");
    
    EXPECT_EQ(int_map.get_size(), 3);
    EXPECT_EQ(*int_map.get(42), "answer");
    EXPECT_EQ(*int_map.get(100), "hundred");
    EXPECT_EQ(*int_map.get(999), "big");
}

TEST_F(HashMapTest, ModifyRetrievedValue) {
    map.insert("key1", 100);
    
    auto* value = map.get("key1");
    ASSERT_NE(value, nullptr);
    *value = 500;
    
    auto* value2 = map.get("key1");
    ASSERT_NE(value2, nullptr);
    EXPECT_EQ(*value2, 500);
}

TEST_F(HashMapTest, ConstGet) {
    map.insert("key1", 100);
    
    const auto& const_map = map;
    auto* value = const_map.get("key1");
    ASSERT_NE(value, nullptr);
    EXPECT_EQ(*value, 100);
}

TEST_F(HashMapTest, CollisionHandling) {
    for (int i = 0; i < 50; ++i) {
        map.insert("key" + std::to_string(i), i * 10);
    }
    
    EXPECT_EQ(map.get_size(), 50);
    
    for (int i = 0; i < 50; ++i) {
        auto* value = map.get("key" + std::to_string(i));
        ASSERT_NE(value, nullptr) << "Failed to find key" << i;
        EXPECT_EQ(*value, i * 10);
    }
}

TEST_F(HashMapTest, DeletedSlotReuse) {
    for (int i = 0; i < 5; ++i) {
        map.insert("key" + std::to_string(i), i);
    }
    
    map.erase("key1");
    map.erase("key3");
    
    EXPECT_EQ(map.get_size(), 3);

    map.insert("newkey1", 1000);
    map.insert("newkey2", 2000);
    
    EXPECT_EQ(map.get_size(), 5);
    EXPECT_TRUE(map.contains("newkey1"));
    EXPECT_TRUE(map.contains("newkey2"));
}

TEST_F(HashMapTest, LargeScaleOperations) {
    const int num_elements = 1000;

    for (int i = 0; i < num_elements; ++i) {
        map.insert("key" + std::to_string(i), i);
    }
    
    EXPECT_EQ(map.get_size(), num_elements);
    
    for (int i = 0; i < num_elements; ++i) {
        auto* value = map.get("key" + std::to_string(i));
        ASSERT_NE(value, nullptr) << "Failed to find key" << i;
        EXPECT_EQ(*value, i);
    }
    
    for (int i = 0; i < num_elements; i += 2) {
        EXPECT_TRUE(map.erase("key" + std::to_string(i)));
    }
    
    EXPECT_EQ(map.get_size(), num_elements / 2);

    for (int i = 1; i < num_elements; i += 2) {
        EXPECT_TRUE(map.contains("key" + std::to_string(i)));
    }
    
    for (int i = 0; i < num_elements; i += 2) {
        EXPECT_FALSE(map.contains("key" + std::to_string(i)));
    }
}

TEST_F(HashMapTest, CopyConstruct) {
    map.insert("key1", 100);
    map.insert("key2", 200);
    
    bmngxn::hash_map<std::string, int> copy = map;
    
    EXPECT_EQ(copy.get_size(), 2);
    EXPECT_EQ(*copy.get("key1"), 100);
    EXPECT_EQ(*copy.get("key2"), 200);
}

TEST_F(HashMapTest, MoveConstruct) {
    map.insert("key1", 100);
    map.insert("key2", 200);
    
    bmngxn::hash_map<std::string, int> moved = std::move(map);
    
    EXPECT_EQ(moved.get_size(), 2);
    EXPECT_EQ(*moved.get("key1"), 100);
    EXPECT_EQ(*moved.get("key2"), 200);
}

TEST_F(HashMapTest, CopyAssignment) {
    map.insert("key1", 100);
    
    bmngxn::hash_map<std::string, int> other;
    other.insert("keyA", 1000);
    
    other = map;
    
    EXPECT_EQ(other.get_size(), 1);
    EXPECT_EQ(*other.get("key1"), 100);
    EXPECT_FALSE(other.contains("keyA"));
}

TEST_F(HashMapTest, MoveAssignment) {
    map.insert("key1", 100);
    
    bmngxn::hash_map<std::string, int> other;
    other.insert("keyA", 1000);
    
    other = std::move(map);
    
    EXPECT_EQ(other.get_size(), 1);
    EXPECT_EQ(*other.get("key1"), 100);
}

TEST_F(HashMapTest, EmptyMap) {
    EXPECT_EQ(map.get_size(), 0);
    EXPECT_GT(map.get_capacity(), 0);
    EXPECT_EQ(map.get("key1"), nullptr);
}

TEST_F(HashMapTest, StringValues) {
    bmngxn::hash_map<int, std::string> str_map;
    
    str_map.insert(1, "apple");
    str_map.insert(2, "banana");
    str_map.insert(3, "cherry");
    
    EXPECT_EQ(str_map.get_size(), 3);
    EXPECT_EQ(*str_map.get(1), "apple");
    EXPECT_EQ(*str_map.get(2), "banana");
    EXPECT_EQ(*str_map.get(3), "cherry");
}
