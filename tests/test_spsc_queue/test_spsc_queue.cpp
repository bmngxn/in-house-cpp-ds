#include "spsc_queue/spsc_queue.h"

#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <chrono>

class SPSCQueueTest : public ::testing::Test {
protected:
    bmngxn::spsc_queue<int, 3> queue;  // capacity_ = 2^3, real capacity = 7
};

TEST_F(SPSCQueueTest, EnqueueAndDequeue) {
    int value = 42;
    EXPECT_TRUE(queue.enqueue(value));
    
    auto result = queue.dequeue();
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 42);
}

TEST_F(SPSCQueueTest, EmptyQueue) {
    EXPECT_TRUE(queue.empty());
    queue.enqueue(1);
    EXPECT_FALSE(queue.empty());
}

TEST_F(SPSCQueueTest, DequeueEmpty) {
    EXPECT_TRUE(queue.empty());
    auto result = queue.dequeue();
    EXPECT_FALSE(result.has_value());
}

TEST_F(SPSCQueueTest, MultipleEnqueueDequeue) {
    for (int i = 0; i < 7; ++i) {
        EXPECT_TRUE(queue.enqueue(i)) << "Failed to enqueue at index " << i;
    }
    
    for (int i = 0; i < 7; ++i) {
        auto result = queue.dequeue();
        EXPECT_TRUE(result.has_value());
        EXPECT_EQ(*result, i);
    }
}

TEST_F(SPSCQueueTest, FullQueue) {
    for (int i = 0; i < 7; ++i) {
        EXPECT_TRUE(queue.enqueue(i));
    }

    EXPECT_FALSE(queue.enqueue(999));
}

TEST_F(SPSCQueueTest, FullQueueThenDequeue) {
    for (int i = 0; i < 7; ++i) {
        EXPECT_TRUE(queue.enqueue(i));
    }

    auto result = queue.dequeue();
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0);
    
    EXPECT_TRUE(queue.enqueue(100));
}

TEST_F(SPSCQueueTest, Wraparound) {
    for (int cycle = 0; cycle < 3; ++cycle) {
        for (int i = 0; i < 7; ++i) {
            EXPECT_TRUE(queue.enqueue(i * 10 + cycle));
        }
        
        for (int i = 0; i < 7; ++i) {
            auto result = queue.dequeue();
            EXPECT_TRUE(result.has_value());
            EXPECT_EQ(*result, i * 10 + cycle);
        }
    }
    
    EXPECT_TRUE(queue.empty());
}

TEST_F(SPSCQueueTest, InterleavedOperations) {
    // Enqueue 3
    queue.enqueue(1);
    queue.enqueue(2);
    queue.enqueue(3);
    
    auto val = queue.dequeue();
    EXPECT_TRUE(val.has_value());
    EXPECT_EQ(*val, 1);
    
    queue.enqueue(4);
    queue.enqueue(5);
    
    val = queue.dequeue();
    EXPECT_EQ(*val, 2);
    val = queue.dequeue();
    EXPECT_EQ(*val, 3);
    val = queue.dequeue();
    EXPECT_EQ(*val, 4);
    val = queue.dequeue();
    EXPECT_EQ(*val, 5);
    
    EXPECT_TRUE(queue.empty());
}

TEST_F(SPSCQueueTest, ConcurrentProducerConsumer) {
    const int num_items = 10000;
    std::vector<int> received;
    received.reserve(num_items);
    
    std::thread producer([this, num_items]() {
        for (int i = 0; i < num_items; ++i) {
            while (!queue.enqueue(i)) {
                // spin til can enqueue
            }
        }
    });
    
    std::thread consumer([this, num_items, &received]() {
        while (received.size() < num_items) {
            if (auto value = queue.dequeue()) {
                received.push_back(*value);
            }
        }
    });
    
    producer.join();
    consumer.join();
    
    EXPECT_EQ(received.size(), num_items);
    for (int i = 0; i < num_items; ++i) {
        EXPECT_EQ(received[i], i);
    }
    EXPECT_TRUE(queue.empty());
}

TEST_F(SPSCQueueTest, MoveSemantics) {
    struct TrackingType {
        int value;
        int move_count = 0;
        
        explicit TrackingType(int v = 0) : value(v) {}
        
        TrackingType(const TrackingType& other) : value(other.value), move_count(other.move_count) {}
        
        TrackingType& operator=(const TrackingType& other) {
            value = other.value;
            move_count = other.move_count;
            return *this;
        }
        
        TrackingType(TrackingType&& other) noexcept : value(other.value), move_count(other.move_count + 1) {}
        
        TrackingType& operator=(TrackingType&& other) noexcept {
            value = other.value;
            move_count = other.move_count + 1;
            return *this;
        }
    };
    
    bmngxn::spsc_queue<TrackingType, 2> move_queue;
    TrackingType item(42);
    
    EXPECT_TRUE(move_queue.enqueue(std::move(item)));
    
    auto result = move_queue.dequeue();
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result->value, 42);
}

TEST_F(SPSCQueueTest, NoCopyConstruct) {
    constexpr bool is_copy = std::is_copy_constructible_v<bmngxn::spsc_queue<int, 3>>;
    EXPECT_FALSE(is_copy);
}

TEST_F(SPSCQueueTest, NoMove) {
    constexpr bool is_move = std::is_move_constructible_v<bmngxn::spsc_queue<int, 3>>;
    EXPECT_FALSE(is_move);
}

TEST_F(SPSCQueueTest, DifferentCapacities) {
    bmngxn::spsc_queue<int, 1> small_queue; 
    bmngxn::spsc_queue<int, 5> large_queue; 
    
    EXPECT_TRUE(small_queue.enqueue(1));
    EXPECT_FALSE(small_queue.enqueue(2));
    
    auto val = small_queue.dequeue();
    EXPECT_TRUE(val.has_value());
    EXPECT_EQ(*val, 1);
    
    for (int i = 0; i < 31; ++i) {
        EXPECT_TRUE(large_queue.enqueue(i));
    }
    EXPECT_FALSE(large_queue.enqueue(999));
}
