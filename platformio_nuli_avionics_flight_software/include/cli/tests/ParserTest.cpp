//
// Created by chris on 1/31/2025.
//

#include <gtest/gtest.h>
#include "cli/Parser.h"

TEST(ParserTest, TestBasic) {
    // basic parsing with a SimpleFlag
    Parser testParserA = Parser();
    SimpleFlag testA("--testA", "some help text", true);
    BaseFlag* testAGroup[] = {&testA};
    testParserA.addFlagGroup(testAGroup);

    EXPECT_FALSE(testA.isSet());
    char argsA[8] = "--testA";
    ASSERT_EQ(testParserA.parse(argsA), 0);
    EXPECT_TRUE(testA.isSet());


    // basic parsing with an ArgumentFlag
    Parser testParserB = Parser();
    ArgumentFlag<int> testB("--testB", 5, "some help text", true);
    BaseFlag* testBGroup[] = {&testB};
    testParserB.addFlagGroup(testBGroup);

    EXPECT_FALSE(testB.isSet());
    char argsB[8] = "--testB";
    ASSERT_EQ(testParserB.parse(argsB), 0);
    EXPECT_TRUE(testB.isSet());
    EXPECT_EQ(testB.getValueDerived(), 5);
}

TEST(ParserTest, TestMultiple) {
    // parsing with multiple SimpleFlags
    Parser testParserA = Parser();
    SimpleFlag testA_1("--testA", "some help text", true);
    SimpleFlag testA_2("-a", "help", true);
    SimpleFlag testA_3("-b", "help", false);
    BaseFlag* testGroupA[] = {&testA_1, &testA_2, &testA_3};
    testParserA.addFlagGroup(testGroupA);

    char argsA[14] = "--testA -a -b";
    ASSERT_EQ(testParserA.parse(argsA), 0);
    // expecting all flags to be set
    EXPECT_TRUE(testA_1.isSet());
    EXPECT_TRUE(testA_2.isSet());
    EXPECT_TRUE(testA_3.isSet());


    // parsing with multiple ArgumentFlags
    Parser testParserB = Parser();
    ArgumentFlag<int> testB_1("--testB", "some help text", true);
    ArgumentFlag<int> testB_2("-a", 0, "some help text", false);
    ArgumentFlag<double> testB_3("-b", "some help text", true);
    BaseFlag* testGroupB[] = {&testB_1, &testB_2, &testB_3};
    testParserB.addFlagGroup(testGroupB);

    char argsB[20] = "--testB 5 -a -b 5.2";
    ASSERT_EQ(testParserB.parse(argsB), 0);
}

TEST(ParserTest, TestHelp) {
    // check help text
    Parser testParserA = Parser();
    SimpleFlag testA_1("--testA", "some help for --testA", true);
    ArgumentFlag<int> testA_2("-a", 0, "provide an integer", false);
    BaseFlag* testGroupA[] = {&testA_1, &testA_2};
    testParserA.addFlagGroup(testGroupA);

    ::testing::internal::CaptureStdout();
    testParserA.printHelp();
    std::string outputA = ::testing::internal::GetCapturedStdout();
    std::string expectedA = "--testA [Required]: some help for --testA\n"
                            "-a [Optional]: provide an integer\n\n";
    EXPECT_EQ(outputA, expectedA);
}

TEST(ParserTest, TestReset) {
    // test that reset unsets a flag
    // right now, it doesn't really do anything else
    Parser testParserA = Parser();
    SimpleFlag testA_1("--testA", "some help for --testA", true);
    ArgumentFlag<int> testA_2("-a", 0, "provide an integer", false);
    BaseFlag* testGroupA[] = {&testA_1, &testA_2};
    testParserA.addFlagGroup(testGroupA);

    char argsA[14] = "--testA -a 5";
    ASSERT_EQ(testParserA.parse(argsA), 0);
    EXPECT_TRUE(testA_1.isSet());
    EXPECT_TRUE(testA_2.isSet());

    // after reset, really the only thing that changes is isSet
    testParserA.resetFlags();
    EXPECT_FALSE(testA_1.isSet());
    EXPECT_FALSE(testA_2.isSet());
}

TEST(ParserTest, TestFailure1) {
    // providing no FlagGroup to parse
    Parser testParserA = Parser();
    char argsA[1] = "";
    ::testing::internal::CaptureStderr();
    ASSERT_EQ(testParserA.parse(argsA), -1);
    std::string outputA = ::testing::internal::GetCapturedStderr();
    std::string expectedA = "No flag group present\n";
    EXPECT_EQ(outputA, expectedA);


    // providing no flags to parse
    Parser testParserB = Parser();
    ArgumentFlag<int> testB_1("--testB", "some help text", true);
    BaseFlag* testGroupB[] = {&testB_1};
    testParserB.addFlagGroup(testGroupB);

    char argsB[1] = "";
    ::testing::internal::CaptureStderr();
    ASSERT_EQ(testParserB.parse(argsB), -1);
    std::string outputB = ::testing::internal::GetCapturedStderr();
    std::string expectedB = "No flag provided\n";
    EXPECT_EQ(outputB, expectedB);
}

TEST(ParserTest, TestFailure2) {
    //@TODO for some reason putting all these tests together killed the runner-- find fix later
    //  maybe fix by adding a way to set the stdout and stderr streams
    // adding flag group, but wrong first flag provided
    Parser testParserC = Parser();
    ArgumentFlag<int> testC_1("--testC", "some help text", true);
    ArgumentFlag<int> testC_2("-a", "some help text", true);
    BaseFlag* testGroupC[] = {&testC_1, &testC_2};
    testParserC.addFlagGroup(testGroupC);

    char argsC[13] = "--flag_group";
    ::testing::internal::CaptureStderr();
    ASSERT_EQ(testParserC.parse(argsC), -1);
    std::string outputC = ::testing::internal::GetCapturedStderr();
    std::string expectedC = "Leader flag not found\n";
    EXPECT_EQ(outputC, expectedC);


    // parsing a unknown flag
    Parser testParserD = Parser();
    ArgumentFlag<int> testD_1("--testD", "some help text", true);
    ArgumentFlag<int> testD_2("-a", "some help text", true);
    BaseFlag* testGroupD[] = {&testD_1, &testD_2};
    testParserD.addFlagGroup(testGroupD);

    char argsD[15] = "--testD 5 -b 2";
    ::testing::internal::CaptureStderr();
    ASSERT_EQ(testParserD.parse(argsD), -1);
    std::string outputD = ::testing::internal::GetCapturedStderr();
    std::string expectedD = "Unknown flag: -b\n";
    EXPECT_EQ(outputD, expectedD);
}

TEST(ParserTest, TestFailure3) {
    // when missing a required argument
    Parser testParserE = Parser();
    SimpleFlag testE_1("--testE", "some help text", true);
    ArgumentFlag<int> testE_2("-a", "some help text", true);
    BaseFlag* testGroupD[] = {&testE_1, &testE_2};
    testParserE.addFlagGroup(testGroupD);

    char argsE[15] = "--testE";
    ::testing::internal::CaptureStderr();
    ASSERT_EQ(testParserE.parse(argsE), -1);
    std::string outputE = ::testing::internal::GetCapturedStderr();
    std::string expectedE = "Missing required argument: -a\n";
    EXPECT_EQ(outputE, expectedE);
}