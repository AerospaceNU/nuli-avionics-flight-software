#include <gtest/gtest.h>
#include "../../src/core/cli/Parser.h"
#include "../../src/core/cli/SimpleFlag.h"
#include "../../src/core/cli/ArgumentFlag.h"
#include "../../src/core/cli/ReturnCodes.h"

//void callback(const char* name, uint8_t* data, uint32_t length, uint8_t group_uid, uint8_t flag_uid, BaseFlag *dependency) { }
void callback() {}


TEST(ParserTest, TestBasic) {
    // basic parsing with a SimpleFlag
    Parser testParserA = Parser();
    SimpleFlag testA("--testA", "some help text", true, 0, callback);
    BaseFlag* testAGroup[] = {&testA};
    testParserA.addFlagGroup(testAGroup);

    EXPECT_FALSE(testA.isSet());
    char argsA[8] = "--testA";
    ASSERT_EQ(testParserA.parse(argsA), 0);
    EXPECT_TRUE(testA.isSet());


    // basic parsing with an ArgumentFlag
    Parser testParserB = Parser();
    ArgumentFlag<int> testB("--testB", 5, "some help text", true, 0, callback);
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
    SimpleFlag testA_1("--testA", "some help text", true, 0, callback);
    SimpleFlag testA_2("-a", "help", true, 0, callback);
    SimpleFlag testA_3("-b", "help", false, 0, callback);
    BaseFlag* testGroupA[] = {&testA_1, &testA_2, &testA_3};
    testParserA.addFlagGroup(testGroupA);

    char argsA[14] = "--testA -a -b";
    ASSERT_EQ(testParserA.parse(argsA), CLI_SUCCESS);
    // expecting all flags to be set
    EXPECT_TRUE(testA_1.isSet());
    EXPECT_TRUE(testA_2.isSet());
    EXPECT_TRUE(testA_3.isSet());


    // parsing with multiple ArgumentFlags
    Parser testParserB = Parser();
    ArgumentFlag<int> testB_1("--testB", "some help text", true, 0, callback);
    ArgumentFlag<int> testB_2("-a", 0, "some help text", false, 0, callback);
    ArgumentFlag<double> testB_3("-b", "some help text", true, 0, callback);
    BaseFlag* testGroupB[] = {&testB_1, &testB_2, &testB_3};
    testParserB.addFlagGroup(testGroupB);

    char argsB[21] = "--testB 5 -a -b -5.2";
    ASSERT_EQ(testParserB.parse(argsB), 0);
}

TEST(ParserTest, TestComplex) {
    Parser testParserA = Parser();
    SimpleFlag testA_1("--testA", "some help text", true, 0, callback);
    ArgumentFlag<int> testA_2("-a", "help", true, 0, callback);
    ArgumentFlag<const char*> testA_3("-b", "help", true, 0, callback);
    SimpleFlag testA_4("-c", "help", true, 0, callback);
    BaseFlag* testGroupA[] = {&testA_1, &testA_2, &testA_3, &testA_4};
    testParserA.addFlagGroup(testGroupA);

    char argsA[33] = "--testA -a 5 -b \"hello world\" -c";
    ASSERT_EQ(testParserA.parse(argsA), 0);

    const char* outputA_3;
    EXPECT_EQ(testParserA.getValue<const char*>("--testA", "-b", outputA_3), 0);
    EXPECT_STREQ(testA_3.getValueDerived(), outputA_3);
    EXPECT_STREQ(outputA_3, "hello world");

}

TEST(ParserTest, TestHelp) {
    // check help text
    Parser testParserA = Parser();
    SimpleFlag testA_1("--testA", "some help for --testA", true, 0, callback);
    ArgumentFlag<int> testA_2("-a", 0, "provide an integer", false, 0, callback);
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
    SimpleFlag testA_1("--testA", "some help for --testA", true, 0, callback);
    ArgumentFlag<int> testA_2("-a", 0, "provide an integer", false, 0, callback);
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
    ASSERT_EQ(testParserA.parse(argsA), CLI_PARSER_NO_FLAG_GROUP_PROVIDED);

    // providing no flags to parse
    Parser testParserB = Parser();
    ArgumentFlag<int> testB_1("--testB", "some help text", true, 0, callback);
    BaseFlag* testGroupB[] = {&testB_1};
    testParserB.addFlagGroup(testGroupB);

    char argsB[1] = "";
    ASSERT_EQ(testParserB.parse(argsB), CLI_PARSER_NO_FLAGS_PROVIDED);
}

TEST(ParserTest, TestFailure2) {
    //@TODO for some reason putting all these tests together killed the runner-- find fix later
    //  maybe fix by adding a way to set the stdout and stderr streams
    // adding flag group, but wrong first flag provided
    Parser testParserC = Parser();
    ArgumentFlag<int> testC_1("--testC", "some help text", true, 0, callback);
    ArgumentFlag<int> testC_2("-a", "some help text", true, 0, callback);
    BaseFlag* testGroupC[] = {&testC_1, &testC_2};
    testParserC.addFlagGroup(testGroupC);

    char argsC[13] = "--flag_group";
    ASSERT_EQ(testParserC.parse(argsC), CLI_PARSER_NO_LEADER_FLAG);

    // parsing a unknown flag
    Parser testParserD = Parser();
    ArgumentFlag<int> testD_1("--testD", "some help text", true, 0, callback);
    ArgumentFlag<int> testD_2("-a", "some help text", true, 0, callback);
    BaseFlag* testGroupD[] = {&testD_1, &testD_2};
    testParserD.addFlagGroup(testGroupD);

    char argsD[15] = "--testD 5 -b 2";
    ASSERT_EQ(testParserD.parse(argsD), CLI_PARSER_UNKNOWN_FLAG);
}

TEST(ParserTest, TestFailure3) {
    // when missing a required argument
    Parser testParserE = Parser();
    SimpleFlag testE_1("--testE", "some help text", true, 0, callback);
    ArgumentFlag<int> testE_2("-a", "some help text", true, 0, callback);
    BaseFlag* testGroupD[] = {&testE_1, &testE_2};
    testParserE.addFlagGroup(testGroupD);

    char argsE[15] = "--testE";
    ASSERT_EQ(testParserE.parse(argsE), CLI_PARSER_MISSING_REQUIRED_ARGS);
}