//
// Created by chris on 1/27/2025.
//

#include <gtest/gtest.h>
#include "cli/BaseFlag.h"
#include "cli/SimpleFlag.h"
#include "cli/ArgumentFlag.h"

// https://google.github.io/googletest/primer.html
// https://github.com/google/googletest/blob/main/googletest/samples/sample3_unittest.cc

// The fixture for testing class Foo.
class FlagTest : public testing::Test {
protected:
    // You can remove any or all of the following functions if their bodies would
    // be empty.

    FlagTest() {
        // You can do set-up work for each test here.
    }

    ~FlagTest() override {
        // You can do clean-up work that doesn't throw exceptions here.
    }

    // If the constructor and destructor are not enough for setting up
    // and cleaning up each test, you can define the following methods:

    void SetUp() override {
        // Code here will be called immediately after the constructor (right
        // before each test).
    }

    void TearDown() override {
        // Code here will be called immediately after each test (right
        // before the destructor).
    }

    // Class members declared here can be used by all tests in the test suite
    // for Foo.
};

void hello(bool a, int8_t b) {

}

void hello2(int a, int8_t b) {

}

void hello3(double a, int8_t b) {

}

void hello4(const char* a, int8_t b) {

}

// Demonstrate some basic assertions.
// TEST(TestSuiteName, TestName)
TEST(SimpleFlag, TestAccessors) {
    SimpleFlag testA("--testA", "some help text", true, hello);    // testing with required flags

    // retrieve basic attributes
    EXPECT_STREQ(testA.name(), "--testA");
    EXPECT_STREQ(testA.help(), "some help text");
    EXPECT_TRUE(testA.isRequired());
    EXPECT_FALSE(testA.isSet());
    EXPECT_FALSE(testA.verify());
}

TEST(SimpleFlag, TestParse) {
    SimpleFlag testA("--testA", "some help text", true, hello);    // testing with required flags

    ASSERT_EQ(testA.parse(nullptr), 0); // should not fail, simple flags have no arguments to parse
    EXPECT_TRUE(testA.isSet());
    EXPECT_EQ(testA.getValueDerived(), true);
    EXPECT_TRUE(testA.verify());
    EXPECT_TRUE(testA.getValueDerived());

    // testing that reset flag only affects the set flag and nothing else.
    testA.reset();
    EXPECT_FALSE(testA.isSet());
    EXPECT_TRUE(testA.isRequired());
    EXPECT_STREQ(testA.name(), "--testA");
    EXPECT_STREQ(testA.help(), "some help text");
    EXPECT_FALSE(testA.getValueDerived());
}

TEST(ArgumentFlag, TestAccessors) {
    // "normal" flag
    ArgumentFlag<int> testA("--testA", 5, "some help text", true, hello2);
    EXPECT_STREQ(testA.name(), "--testA");
    EXPECT_STREQ(testA.help(), "some help text");
    EXPECT_TRUE(testA.isRequired());
    EXPECT_FALSE(testA.isSet());
    EXPECT_FALSE(testA.verify());

    // special case of const char* input
    ArgumentFlag<const char*> testB("--testB", "defaultTest", "some help text", false, hello4);
    EXPECT_STREQ(testB.name(), "--testB");
    EXPECT_STREQ(testB.help(), "some help text");
    EXPECT_FALSE(testB.isRequired());
    EXPECT_FALSE(testB.isSet());
    EXPECT_TRUE(testB.verify());

    // no default value provided
    ArgumentFlag<double> testC("--testC", "some help text", true, hello3);
    EXPECT_STREQ(testC.name(), "--testC");
    EXPECT_STREQ(testC.help(), "some help text");
    EXPECT_TRUE(testC.isRequired());
    EXPECT_FALSE(testC.isSet());
    EXPECT_FALSE(testC.verify());
}

TEST(ArgumentFlag, TestParse) {
    // "normal" flag, forced to use default
    ArgumentFlag<int> testA("--testA", 0, "some help text", true, hello2);

    ASSERT_EQ(testA.parse(nullptr), 0); // should not fail when given no argument due to default argument
    EXPECT_TRUE(testA.isSet());
    EXPECT_EQ(testA.getValueDerived(), 0);
    EXPECT_TRUE(testA.verify());

    // testing that reset flag only affects the set flag and nothing else.
    testA.reset();
    EXPECT_FALSE(testA.isSet());
    EXPECT_TRUE(testA.isRequired());
    EXPECT_STREQ(testA.name(), "--testA");
    EXPECT_STREQ(testA.help(), "some help text");


    // special case of const char* input
    ArgumentFlag<const char*> testB("--testB", "defaultTest", "some help text", true, hello4);
    char argB[12] = "changedText";

    ASSERT_EQ(testB.parse(argB), 0); // should not fail, argument given
    EXPECT_TRUE(testB.isSet());
    EXPECT_STREQ(testB.getValueDerived(), "changedText");


    // no default value provided
    ArgumentFlag<double> testC("--testC", "some help text", true, hello3);
    char argC[4] = "5.1";

    ASSERT_EQ(testC.parse(argC), 0); // should not fail, no default argument but argument provided
    EXPECT_TRUE(testC.isSet());
    EXPECT_EQ(testC.getValueDerived(), 5.1);
}

TEST(ArgumentFlag, TestFailure) {
    // no default argument passed in
    ArgumentFlag<int> testA("--testA", "some help text", true, hello2);
    ::testing::internal::CaptureStderr();
    ASSERT_EQ(testA.parse(nullptr), -1); // should fail, no default argument provided and no argument provided
    std::string outputA = ::testing::internal::GetCapturedStderr();
    std::string expectedA = "Default argument not set, value required for --testA\n";
    EXPECT_EQ(outputA, expectedA);

    // parsing error, wrong type input
    ArgumentFlag<int> testB("--testB", 0, "some help text", true, hello2);
    char argB[4] = "abc";
    ::testing::internal::CaptureStderr();
    ASSERT_EQ(testB.parse(argB), -1);
    std::string outputB = ::testing::internal::GetCapturedStderr();
    std::string expectedB = "Failed to parse argument: abc\n";
    EXPECT_EQ(outputB, expectedB);

    // parsing error, mixed input
    ArgumentFlag<double> testC("--testC", 5.0, "some help text", true, hello3);
    char argC[7] = "123abc";
    ::testing::internal::CaptureStderr();
    ASSERT_EQ(testC.parse(argC), -1); // should fail, argument provided cannot map to a fundamental type (or const char*)
    std::string outputC = ::testing::internal::GetCapturedStderr();
    std::string expectedC = "Failed to parse argument (extra characters found): 123abc\n";
    EXPECT_EQ(outputC, expectedC);
}