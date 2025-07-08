#include <gtest/gtest.h>
#include "../../src/core/cli/BaseFlag.h"
#include "../../src/core/cli/SimpleFlag.h"
#include "../../src/core/cli/ArgumentFlag.h"

// https://google.github.io/googletest/primer.html
// https://github.com/google/googletest/blob/main/googletest/samples/sample3_unittest.cc

void callback(const char* name, uint8_t* data, uint32_t length, uint8_t group_uid, uint8_t flag_uid, BaseFlag *dependency) { }


// Demonstrate some basic assertions.
// TEST(TestSuiteName, TestName)
TEST(SimpleFlag, TestAccessors) {
    SimpleFlag testA("--testA", "some help text", true, 0, callback);    // testing with required flags

    // retrieve basic attributes
    EXPECT_STREQ(testA.name(), "--testA");
    EXPECT_STREQ(testA.help(), "some help text");
    EXPECT_TRUE(testA.isRequired());
    EXPECT_FALSE(testA.isSet());
    EXPECT_FALSE(testA.verify());
}

TEST(SimpleFlag, TestParse) {
    SimpleFlag testA("--testA", "some help text", true, 0, callback);    // testing with required flags

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
    ArgumentFlag<int> testA("--testA", 5, "some help text", true, 0, callback);
    EXPECT_STREQ(testA.name(), "--testA");
    EXPECT_STREQ(testA.help(), "some help text");
    EXPECT_TRUE(testA.isRequired());
    EXPECT_FALSE(testA.isSet());
    EXPECT_FALSE(testA.verify());

    // special case of const char* input
    ArgumentFlag<const char*> testB("--testB", "defaultTest", "some help text", false, 0, callback);
    EXPECT_STREQ(testB.name(), "--testB");
    EXPECT_STREQ(testB.help(), "some help text");
    EXPECT_FALSE(testB.isRequired());
    EXPECT_FALSE(testB.isSet());
    EXPECT_TRUE(testB.verify());

    // no default value provided
    ArgumentFlag<double> testC("--testC", "some help text", true, 0, callback);
    EXPECT_STREQ(testC.name(), "--testC");
    EXPECT_STREQ(testC.help(), "some help text");
    EXPECT_TRUE(testC.isRequired());
    EXPECT_FALSE(testC.isSet());
    EXPECT_FALSE(testC.verify());
}

TEST(ArgumentFlag, TestParse) {
    // "normal" flag, forced to use default
    ArgumentFlag<int> testA("--testA", 0, "some help text", true, 0, callback);

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
    ArgumentFlag<const char*> testB("--testB", "defaultTest", "some help text", true, 0, callback);
    char argB[12] = "changedText";

    ASSERT_EQ(testB.parse(argB), 0); // should not fail, argument given
    EXPECT_TRUE(testB.isSet());
    EXPECT_STREQ(testB.getValueDerived(), "changedText");


    // no default value provided
    ArgumentFlag<double> testC("--testC", "some help text", true, 0, callback);
    char argC[4] = "5.1";

    ASSERT_EQ(testC.parse(argC), 0); // should not fail, no default argument but argument provided
    EXPECT_TRUE(testC.isSet());
    EXPECT_EQ(testC.getValueDerived(), 5.1);
}

TEST(ArgumentFlag, TestFailure) {
    // no default argument passed in
    ArgumentFlag<int> testA("--testA", "some help text", true, 0, callback);
    ::testing::internal::CaptureStderr();
    ASSERT_EQ(testA.parse(nullptr), -1); // should fail, no default argument provided and no argument provided
    std::string outputA = ::testing::internal::GetCapturedStderr();
    std::string expectedA = "Default argument not set, value required for --testA\n";
    EXPECT_EQ(outputA, expectedA);

    // parsing error, wrong type input
    ArgumentFlag<int> testB("--testB", 0, "some help text", true, 0, callback);
    char argB[4] = "abc";
    ::testing::internal::CaptureStderr();
    ASSERT_EQ(testB.parse(argB), -1);
    std::string outputB = ::testing::internal::GetCapturedStderr();
    std::string expectedB = "Failed to parse argument: abc\n";
    EXPECT_EQ(outputB, expectedB);

    // parsing error, mixed input
    ArgumentFlag<double> testC("--testC", 5.0, "some help text", true, 0, callback);
    char argC[7] = "123abc";
    ::testing::internal::CaptureStderr();
    ASSERT_EQ(testC.parse(argC), -1); // should fail, argument provided cannot map to a fundamental type (or const char*)
    std::string outputC = ::testing::internal::GetCapturedStderr();
    std::string expectedC = "Failed to parse argument (extra characters found): 123abc\n";
    EXPECT_EQ(outputC, expectedC);
}