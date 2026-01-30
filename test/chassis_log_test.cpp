#include "chassis_log.hpp"

#include <gtest/gtest.h>

TEST(ChassisLogContextTest, CanConstructWithChassisName)
{
    EXPECT_NO_THROW({ ChassisLogContext logger{"chassis0"}; });
}

TEST(ChassisLogContextTest, BasicLoggingCallsDoNotThrow)
{
    ChassisLogContext logger{"chassis0"};

    EXPECT_NO_THROW(logger.info("info message"));
    EXPECT_NO_THROW(logger.debug("debug message"));
    EXPECT_NO_THROW(logger.warning("warning message"));
    EXPECT_NO_THROW(logger.error("error message"));
}

TEST(ChassisLogContextTest, StructuredArgumentsAreAccepted)
{
    ChassisLogContext logger{"chassis0"};

    EXPECT_NO_THROW(logger.info("PSU={PSU} STATE={STATE}", "PSU", "psu0",
                                "STATE", "Present"));

    EXPECT_NO_THROW(logger.error("Failure CODE={CODE}", "CODE", 42));
}

TEST(ChassisLogContextTest, SupportsDifferentMessageTypes)
{
    ChassisLogContext logger{"chassis0"};

    const char* cstrMsg = "c-string message";
    std::string stringMsg = "std::string message";

    EXPECT_NO_THROW(logger.info(cstrMsg));
    EXPECT_NO_THROW(logger.info(stringMsg));
}

TEST(ChassisLogContextTest, StructuredArgumentsAreAccepted)
{
    ChassisLogContext logger{"chassis-test"};

    EXPECT_NO_THROW(logger.info("PSU={PSU} STATE={STATE}", "PSU", "psu0",
                                "STATE", "Present"));

    EXPECT_NO_THROW(logger.error("Failure CODE={CODE}", "CODE", 42));
}

TEST(ChassisLogContextTest, PerfectForwardingWorks)
{
    ChassisLogContext logger{std::string{"chassis-forward"}};

    std::string dynamicMsg = "dynamic message";
    std::string key = "KEY";
    std::string value = "VALUE";

    EXPECT_NO_THROW(logger.info(dynamicMsg, key.c_str(), value));
}
