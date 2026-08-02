#include <gtest/gtest.h>

#include <cstdio>

#include "ExpressionStore.h"

namespace {
RocketState_s makeState(uint32_t runtimeMs, float altitudeM = 0, float velocityMS = 0) {
    RocketState_s state{};
    state.timestamp.runtime_ms = runtimeMs;
    state.state1D.altitudeM = altitudeM;
    state.state1D.velocityMS = velocityMS;
    return state;
}
} // namespace

class ExpressionStoreTest : public ::testing::Test {
protected:
    ExpressionStore store;
};

TEST_F(ExpressionStoreTest, ParsesConstantComparison) {
    uint8_t rootId;
    const ExpressionValueType_e type = store.compile(0, "(altitudeM < 50)", &rootId);
    ASSERT_EQ(type, ExpressionValueType_e::Boolean);

    store.tick(makeState(0, 10));
    EXPECT_TRUE(store.getBooleanValue(rootId));

    store.tick(makeState(1, 100));
    EXPECT_FALSE(store.getBooleanValue(rootId));
}

TEST_F(ExpressionStoreTest, RejectsUnknownVariable) {
    uint8_t rootId;
    EXPECT_EQ(store.compile(0, "(bogusVar < 50)", &rootId), ExpressionValueType_e::Invalid);
}

TEST_F(ExpressionStoreTest, RejectsTypeMismatch) {
    // "and" requires boolean operands; altitudeM/3 are both numbers.
    uint8_t rootId;
    EXPECT_EQ(store.compile(0, "(altitudeM and 3)", &rootId), ExpressionValueType_e::Invalid);
}

TEST_F(ExpressionStoreTest, RejectsMalformedInput) {
    uint8_t rootId;
    EXPECT_EQ(store.compile(0, "(altitudeM < )", &rootId), ExpressionValueType_e::Invalid);
    EXPECT_EQ(store.compile(0, "", &rootId), ExpressionValueType_e::Invalid);
    EXPECT_EQ(store.compile(0, "(a b c d)", &rootId), ExpressionValueType_e::Invalid);
}

TEST_F(ExpressionStoreTest, AndOrNot) {
    uint8_t rootId;
    const ExpressionValueType_e type =
        store.compile(0, "((altitudeM < 50) and (not (velocityMS > 0)))", &rootId);
    ASSERT_EQ(type, ExpressionValueType_e::Boolean);

    store.tick(makeState(0, 10, -5)); // altitude<50 true, velocity>0 false -> not->true -> and true
    EXPECT_TRUE(store.getBooleanValue(rootId));

    store.tick(makeState(1, 10, 5)); // velocity>0 true -> not->false -> and false
    EXPECT_FALSE(store.getBooleanValue(rootId));
}

TEST_F(ExpressionStoreTest, ArithmeticFunctions) {
    uint8_t rootId;
    const ExpressionValueType_e type = store.compile(0, "((altitudeM + 5.00) > 10.00)", &rootId);
    ASSERT_EQ(type, ExpressionValueType_e::Boolean);

    store.tick(makeState(0, 10));
    EXPECT_TRUE(store.getBooleanValue(rootId)); // 10+5=15 > 10

    store.tick(makeState(1, 2));
    EXPECT_FALSE(store.getBooleanValue(rootId)); // 2+5=7 > 10 is false
}

TEST_F(ExpressionStoreTest, RoundTripToString) {
    uint8_t rootId;
    const char* text = "((altitudeM < 50.00) and (velocityMS > 0.00))";
    ASSERT_EQ(store.compile(0, text, &rootId), ExpressionValueType_e::Boolean);

    char buffer[128];
    store.conditionToString(rootId, buffer, sizeof(buffer));
    EXPECT_STREQ(buffer, text);
}

TEST_F(ExpressionStoreTest, ForRequiresSustainedDuration) {
    uint8_t rootId;
    const ExpressionValueType_e type = store.compile(0, "((velocityMS < 0) for 2.00)", &rootId);
    ASSERT_EQ(type, ExpressionValueType_e::Boolean);

    store.tick(makeState(0, 0, -1));
    EXPECT_FALSE(store.getBooleanValue(rootId)) << "condition just started";

    store.tick(makeState(1000, 0, -1));
    EXPECT_FALSE(store.getBooleanValue(rootId)) << "only ~1s sustained so far";

    store.tick(makeState(2500, 0, -1));
    EXPECT_TRUE(store.getBooleanValue(rootId)) << "sustained > 2s";

    store.tick(makeState(2600, 0, 1)); // condition breaks
    EXPECT_FALSE(store.getBooleanValue(rootId));

    store.tick(makeState(5000, 0, -1)); // true again, but the streak just restarted
    EXPECT_FALSE(store.getBooleanValue(rootId));
}

TEST_F(ExpressionStoreTest, AfterMeasuresTimeSinceFirstTrue) {
    uint8_t rootId;
    // True once 2s have passed since velocityMS>0 FIRST became true, even if it's since gone false.
    const ExpressionValueType_e type = store.compile(0, "(2.00 after (velocityMS > 0))", &rootId);
    ASSERT_EQ(type, ExpressionValueType_e::Boolean);

    store.tick(makeState(0, 0, 5)); // velocityMS>0 first true at t=0
    EXPECT_FALSE(store.getBooleanValue(rootId));

    store.tick(makeState(1000, 0, -5)); // goes false again, but firstTrue stays at t=0
    EXPECT_FALSE(store.getBooleanValue(rootId));

    store.tick(makeState(2500, 0, -5));
    EXPECT_TRUE(store.getBooleanValue(rootId)) << "2.5s have passed since it first became true";
}

TEST_F(ExpressionStoreTest, EverLatchesTrueForever) {
    uint8_t rootId;
    ASSERT_EQ(store.compile(0, "(ever (altitudeM > 100))", &rootId), ExpressionValueType_e::Boolean);

    store.tick(makeState(0, 10));
    EXPECT_FALSE(store.getBooleanValue(rootId));

    store.tick(makeState(1, 150));
    EXPECT_TRUE(store.getBooleanValue(rootId));

    store.tick(makeState(2, 10)); // condition no longer true, but the latch stays tripped
    EXPECT_TRUE(store.getBooleanValue(rootId));
}

TEST_F(ExpressionStoreTest, AlwaysLatchesFalseForever) {
    uint8_t rootId;
    ASSERT_EQ(store.compile(0, "(always (altitudeM < 100))", &rootId), ExpressionValueType_e::Boolean);

    store.tick(makeState(0, 10));
    EXPECT_TRUE(store.getBooleanValue(rootId));

    store.tick(makeState(1, 150)); // condition false once -> latch trips false
    EXPECT_FALSE(store.getBooleanValue(rootId));

    store.tick(makeState(2, 10)); // condition true again, but the latch stays tripped
    EXPECT_FALSE(store.getBooleanValue(rootId));
}

TEST_F(ExpressionStoreTest, BadRecompileLeavesOldTriggerIntact) {
    uint8_t rootId;
    ASSERT_EQ(store.compile(3, "(altitudeM < 50)", &rootId), ExpressionValueType_e::Boolean);

    store.tick(makeState(0, 10));
    EXPECT_TRUE(store.getBooleanValue(rootId));

    uint8_t badId;
    EXPECT_EQ(store.compile(3, "(altitudeM < )", &badId), ExpressionValueType_e::Invalid);

    // Same owner, same root id: the failed recompile must not have disturbed it.
    store.tick(makeState(1, 10));
    EXPECT_TRUE(store.getBooleanValue(rootId));
    store.tick(makeState(2, 100));
    EXPECT_FALSE(store.getBooleanValue(rootId));
}

TEST_F(ExpressionStoreTest, RemoveOwnerFreesItsNodes) {
    uint8_t rootId;
    ASSERT_EQ(store.compile(7, "(altitudeM < 50)", &rootId), ExpressionValueType_e::Boolean);

    store.removeOwner(7);

    // A freed id reads back as false/empty, never as leftover garbage or a crash.
    EXPECT_FALSE(store.getBooleanValue(rootId));
    char buffer[32];
    store.conditionToString(rootId, buffer, sizeof(buffer));
    EXPECT_STREQ(buffer, "");
}

TEST_F(ExpressionStoreTest, RecompileReplacesPreviousOwnerTree) {
    uint8_t firstRootId;
    ASSERT_EQ(store.compile(1, "(altitudeM < 50)", &firstRootId), ExpressionValueType_e::Boolean);

    uint8_t secondRootId;
    ASSERT_EQ(store.compile(1, "(altitudeM < 200)", &secondRootId), ExpressionValueType_e::Boolean);

    store.tick(makeState(0, 100));
    EXPECT_TRUE(store.getBooleanValue(secondRootId)); // 100 < 200
    // The old tree's root id was freed by the second compile() for the same owner.
    EXPECT_FALSE(store.getBooleanValue(firstRootId));
}

TEST_F(ExpressionStoreTest, PoolExhaustionFailsCleanlyWithoutCorruptingExistingTriggers) {
    // Each "(altitudeM < N)" condition allocates 3 nodes (var + const + binary).
    // ExpressionStore::MAX_EXPRESSIONS == 32, so 10 successful compiles use 30, leaving room for
    // only 2 of the 11th's 3 nodes - deliberately exercising the rollback path, not just a clean
    // "no room at all" rejection.
    uint8_t rootIds[10];
    for (uint8_t i = 0; i < 10; ++i) {
        char text[32];
        snprintf(text, sizeof(text), "(altitudeM < %d)", 100 + i); // thresholds all comfortably above the tick's altitude
        ASSERT_EQ(store.compile(i, text, &rootIds[i]), ExpressionValueType_e::Boolean) << "trigger " << int(i);
    }

    uint8_t overflowId;
    EXPECT_EQ(store.compile(200, "(altitudeM < 999)", &overflowId), ExpressionValueType_e::Invalid);

    store.tick(makeState(0, 5));
    for (uint8_t i = 0; i < 10; ++i) {
        EXPECT_TRUE(store.getBooleanValue(rootIds[i])) << "trigger " << int(i) << " should be unaffected";
    }
}
