#include "unique_ptr/unique_ptr.h"
#include <gtest/gtest.h>

/**
 * Checks for double deletes / memory leaks
 */
struct Resource {
    static int alive_count_;
    int value_;

    Resource(int value = 0) 
        : value_(value) 
    { 
        alive_count_++; 
    }

    ~Resource() { 
        alive_count_--; 
    }

    int foo() const { 
        return value_; 
    }
};
int Resource::alive_count_ = 0;

class UniquePtrTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset the resource before every test 
        Resource::alive_count_ = 0; 
    }

    void TearDown() override {
        // Ensure no leaks after tests
        EXPECT_EQ(Resource::alive_count_, 0) << "Memory leak/double delete detected";
    }
};

TEST_F(UniquePtrTest, CreateAndAccessTest) {
    Resource* raw_ptr = new Resource(36);
    bmngxn::unique_ptr<Resource> p1(raw_ptr);

    EXPECT_EQ(p1->value_, 36);
    EXPECT_EQ(p1.get(), raw_ptr);
    EXPECT_EQ(Resource::alive_count_, 1);

    bmngxn::unique_ptr<Resource> p2(new Resource(67));
    EXPECT_EQ((*p2).value_, 67);
    EXPECT_NE(p2, nullptr);
    EXPECT_EQ(Resource::alive_count_, 2);
}

TEST_F(UniquePtrTest, MoveConstructorTest) {
    bmngxn::unique_ptr<Resource> p1(new Resource(36));
    bmngxn::unique_ptr<Resource> p2(std::move(p1)); 

    EXPECT_EQ(p2->value_, 36);
    EXPECT_NE(p2, nullptr);
    EXPECT_EQ(p1, nullptr); 
    EXPECT_EQ(Resource::alive_count_, 1);
}

TEST_F(UniquePtrTest, MoveAssignmentTest) {
    bmngxn::unique_ptr<Resource> p1(new Resource(36));
    bmngxn::unique_ptr<Resource> p2(new Resource(67));
    
    EXPECT_EQ(Resource::alive_count_, 2);

    p1 = std::move(p2);

    EXPECT_NE(p1, nullptr);
    EXPECT_EQ(p1->value_, 67);
    EXPECT_EQ(p2, nullptr);
    EXPECT_EQ(Resource::alive_count_, 1); 
}

TEST_F(UniquePtrTest, ReleaseTest) {
    bmngxn::unique_ptr<Resource> ptr(new Resource(36));
    Resource* rawPtr = ptr.release();

    EXPECT_EQ(ptr, nullptr);
    EXPECT_NE(rawPtr, nullptr);
    EXPECT_EQ(rawPtr->value_, 36);
    EXPECT_EQ(Resource::alive_count_, 1);

    delete rawPtr; 
}

TEST_F(UniquePtrTest, ResetUniquePtr) {
    bmngxn::unique_ptr<Resource> ptr(new Resource(36));
    EXPECT_EQ(Resource::alive_count_, 1);

    ptr.reset(new Resource(67));
    EXPECT_NE(ptr, nullptr);
    EXPECT_EQ(ptr->value_, 67);
    EXPECT_EQ(Resource::alive_count_, 1); 

    ptr.reset(ptr.get());
    EXPECT_EQ(ptr->value_, 67);

    ptr.reset();
    EXPECT_EQ(ptr, nullptr);
    EXPECT_EQ(Resource::alive_count_, 0);
}

TEST_F(UniquePtrTest, SwapTest) {
    Resource* first = new Resource(36);
    Resource* second = new Resource(67);

    bmngxn::unique_ptr<Resource> p1(first);
    bmngxn::unique_ptr<Resource> p2(second);

    p1.swap(p2);

    EXPECT_EQ(p2.get(), first);
    EXPECT_EQ(p1.get(), second);
    EXPECT_EQ(p1->value_, 67);
    EXPECT_EQ(p2->value_, 36);
}


TEST_F(UniquePtrTest, OperatorBoolTest) {
    bmngxn::unique_ptr<Resource> p1;
    bmngxn::unique_ptr<Resource> p2(new Resource(36));

    EXPECT_FALSE(p1);
    EXPECT_TRUE(p2);
}

TEST_F(UniquePtrTest, PointerToArrayConstructionAndAccess) {
    {
        bmngxn::unique_ptr<Resource[]> p(new Resource[3]{Resource(1), Resource(2), Resource(3)});

        EXPECT_NE(p, nullptr);
        EXPECT_EQ(p[0].value_, 1);
        EXPECT_EQ(p[2].value_, 3);
        EXPECT_EQ(Resource::alive_count_, 3);
    } 
    // array now goes out of scope. 
    // -> TearDown() will verify alive_count_ is 0, proving delete[] was called safely
}


TEST_F(UniquePtrTest, EmptyBaseOptimizationTest) {
    EXPECT_EQ(sizeof(bmngxn::unique_ptr<int>), sizeof(int*));
}