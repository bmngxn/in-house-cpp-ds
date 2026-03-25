#include "shared_ptr/shared_ptr.h"

#include <gtest/gtest.h>

#include <thread>
#include <utility>
#include <vector>

struct SharedResource {
    static int alive_count_;
    int value_;

    explicit SharedResource(int value = 0)
        : value_(value)
    {
        ++alive_count_;
    }

    ~SharedResource() {
        --alive_count_;
    }

    int foo() const {
        return value_;
    }
};

int SharedResource::alive_count_ = 0;

class SharedPtrTest : public ::testing::Test {
protected:
    void SetUp() override {
        SharedResource::alive_count_ = 0;
    }

    void TearDown() override {
        EXPECT_EQ(SharedResource::alive_count_, 0) << "Memory leak/double delete detected";
    }
};

TEST_F(SharedPtrTest, CreateAndAccessTest) {
    SharedResource* raw_ptr = new SharedResource(36);
    bmngxn::shared_ptr<SharedResource> p1(raw_ptr);

    EXPECT_EQ(p1.get(), raw_ptr);
    EXPECT_EQ(p1->value_, 36);
    EXPECT_EQ((*p1).value_, 36);
    EXPECT_EQ(p1.use_count(), 1);
    EXPECT_TRUE(p1.unique());
}

TEST_F(SharedPtrTest, CopyConstructorIncrementsUseCount) {
    bmngxn::shared_ptr<SharedResource> p1(new SharedResource(36));
    bmngxn::shared_ptr<SharedResource> p2(p1);

    EXPECT_EQ(p1.get(), p2.get());
    EXPECT_EQ(p1.use_count(), 2);
    EXPECT_EQ(p2.use_count(), 2);
    EXPECT_EQ(SharedResource::alive_count_, 1);
}

TEST_F(SharedPtrTest, CopyAssignmentSharesOwnership) {
    bmngxn::shared_ptr<SharedResource> p1(new SharedResource(36));
    bmngxn::shared_ptr<SharedResource> p2(new SharedResource(67));

    EXPECT_EQ(SharedResource::alive_count_, 2);

    p2 = p1;

    EXPECT_EQ(p1.get(), p2.get());
    EXPECT_EQ(p1.use_count(), 2);
    EXPECT_EQ(p2.use_count(), 2);
    EXPECT_EQ(SharedResource::alive_count_, 1);
}

TEST_F(SharedPtrTest, MoveConstructorTransfersOwnership) {
    bmngxn::shared_ptr<SharedResource> p1(new SharedResource(36));
    bmngxn::shared_ptr<SharedResource> p2(std::move(p1));

    EXPECT_EQ(p1, nullptr);
    EXPECT_NE(p2, nullptr);
    EXPECT_EQ(p2->value_, 36);
    EXPECT_EQ(p2.use_count(), 1);
}

TEST_F(SharedPtrTest, MoveAssignmentTransfersOwnership) {
    bmngxn::shared_ptr<SharedResource> p1(new SharedResource(36));
    bmngxn::shared_ptr<SharedResource> p2(new SharedResource(67));

    EXPECT_EQ(SharedResource::alive_count_, 2);

    p2 = std::move(p1);

    EXPECT_EQ(p1, nullptr);
    EXPECT_NE(p2, nullptr);
    EXPECT_EQ(p2->value_, 36);
    EXPECT_EQ(p2.use_count(), 1);
    EXPECT_EQ(SharedResource::alive_count_, 1);
}

TEST_F(SharedPtrTest, ResetDecrementsUseCount) {
    bmngxn::shared_ptr<SharedResource> p1(new SharedResource(36));
    bmngxn::shared_ptr<SharedResource> p2(p1);

    p1.reset();

    EXPECT_EQ(p1, nullptr);
    EXPECT_NE(p2, nullptr);
    EXPECT_EQ(p2.use_count(), 1);
    EXPECT_EQ(SharedResource::alive_count_, 1);
}

TEST_F(SharedPtrTest, ResetToNewPointerReplacesOwnership) {
    bmngxn::shared_ptr<SharedResource> ptr(new SharedResource(36));

    ptr.reset(new SharedResource(67));

    EXPECT_NE(ptr, nullptr);
    EXPECT_EQ(ptr->value_, 67);
    EXPECT_EQ(ptr.use_count(), 1);
    EXPECT_EQ(SharedResource::alive_count_, 1);
}

TEST_F(SharedPtrTest, SwapExchangesControlBlocks) {
    bmngxn::shared_ptr<SharedResource> p1(new SharedResource(36));
    bmngxn::shared_ptr<SharedResource> p2(new SharedResource(67));

    p1.swap(p2);

    EXPECT_EQ(p1->value_, 67);
    EXPECT_EQ(p2->value_, 36);
    EXPECT_EQ(p1.use_count(), 1);
    EXPECT_EQ(p2.use_count(), 1);
}


TEST_F(SharedPtrTest, LastOwnerDestroysResource) {
    {
        bmngxn::shared_ptr<SharedResource> p1(new SharedResource(36));
        {
            bmngxn::shared_ptr<SharedResource> p2(p1);
            EXPECT_EQ(SharedResource::alive_count_, 1);
            EXPECT_EQ(p1.use_count(), 2);
        }

        EXPECT_EQ(SharedResource::alive_count_, 1);
        EXPECT_EQ(p1.use_count(), 1);
    }

    EXPECT_EQ(SharedResource::alive_count_, 0);
}

TEST_F(SharedPtrTest, ConcurrentCopiesAreReferenceCountSafe) {
    bmngxn::shared_ptr<SharedResource> shared(new SharedResource(36));

    constexpr int kThreadCount = 8;
    constexpr int kCopiesPerThread = 1000;

    std::vector<std::thread> threads;
    threads.reserve(kThreadCount);

    for (int i = 0; i < kThreadCount; i++) {
        threads.emplace_back([shared]() mutable {
            for (int j = 0; j < kCopiesPerThread; j++) {
                bmngxn::shared_ptr<SharedResource> local(shared);
                EXPECT_NE(local, nullptr);
                EXPECT_EQ(local->value_, 36);
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(shared.use_count(), 1);
    EXPECT_EQ(SharedResource::alive_count_, 1);
}
