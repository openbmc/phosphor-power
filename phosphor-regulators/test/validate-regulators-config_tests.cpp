/**
 * Copyright © 2020 IBM Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <fstream>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#define EXPECT_FILE_VALID(tmpFile) expectFileValid(tmpFile)
#define EXPECT_FILE_INVALID(tmpFile, expectedErrorMessage, expectedOutputMessage) \
        expectFileInvalid(tmpFile, expectedErrorMessage, expectedOutputMessage)

using json = nlohmann::json;

std::string createTmpFile()
{
    // create temporary file using mkstemp under /tmp/. random name for XXXXXX
    char file_name[] = "/tmp/temp-XXXXXX";
    int fd = mkstemp(file_name);
    if (fd == -1)
    {
        perror("Can't create temporary file");
    }
    close(fd);
    return file_name;
}

std::string getValidationToolCommand(const std::string& config_file_name)
{
    std::string command = "python ../tools/validate-regulators-config.py -s \
                           ../schema/config_schema.json -c ";
    command += config_file_name;
    return command;
}

std::string runToolStderr(const std::string& config_file_name)
{
    // run the validation tool with the temporary file and return the output
    // of the validation tool.
    std::string command = getValidationToolCommand(config_file_name);

    // only get the stderr from validation tool.
    command += " 2>&1 >/dev/null";

    char buffer[128];
    std::string result = "";
    // to get the stderr from the validation tool.
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe)
    {
        throw std::runtime_error("popen() failed!");
    }
    if (fgets(buffer, sizeof buffer, pipe) != NULL)
    {
        result = buffer;
    }
    pclose(pipe);
    return result;
}

std::string runToolStdout(const std::string& config_file_name)
{
    // run the validation tool with the temporary file and return the output
    // of the validation tool.
    std::string command = getValidationToolCommand(config_file_name);

    // get the jsonschema print from validation tool.
    char buffer[256];
    std::string result = "";
    // to get the stdout from the validation tool.
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe)
    {
        throw std::runtime_error("popen() failed!");
    }
    if (fgets(buffer, sizeof buffer, pipe) != NULL)
    {
        result = buffer;
    }
    pclose(pipe);
    return result;
}

int runToolExitCode(const std::string& config_file_name)
{
    // run the validation tool with the temporary file and return the output
    // of the validation tool.
    std::string command = getValidationToolCommand(config_file_name);

    // to get the exit code for the validation tool.
    int exitCode = system(command.c_str());
    return exitCode;
}

void expectFileValid(const std::string& tmpFile)
{
    EXPECT_EQ(runToolExitCode(tmpFile), 0);
    EXPECT_EQ(runToolStderr(tmpFile), "");
    EXPECT_EQ(runToolStdout(tmpFile), "");
}

void expectFileInvalid(const std::string& tmpFile, \
                       const std::string& expectedErrorMessage, \
                       const std::string& expectedOutputMessage)
{
    EXPECT_NE(runToolExitCode(tmpFile), 0);
    EXPECT_EQ(runToolStderr(tmpFile), expectedErrorMessage);
    EXPECT_EQ(runToolStdout(tmpFile), expectedOutputMessage);
}

// create json data for "rule" object
json createJsonObjectRule()
{
    json json_data;

    // create json data for rule
    json_data["comments"] = {"Config file for a FooBar one-chassis system"};
    json_data["rules"][0]["comments"] = {"Sets output voltage for a PMBus regulator rail"};
    json_data["rules"][0]["id"] = "set_voltage_rule";
    json_data["rules"][0]["actions"][0]["pmbus_write_vout_command"]["format"] = "linear";
    json_data["chassis"][0]["number"] = 1;

    return json_data;
}

TEST(ValidateRegulatorsConfigTest, Rule)
{

    std::string file_name_ ;

    // valid test comments property, id property,
    // action property specified.
    {

        file_name_ = createTmpFile();
        json json_data = createJsonObjectRule();
        std::string string_json = json_data.dump();
        // write json data to tmp file.
        std::ofstream out(file_name_);
        out << string_json;
        out.close();

        EXPECT_FILE_VALID(file_name_);
        unlink(file_name_.c_str());
    }

    // valid test rule with no comments
    {

        file_name_ = createTmpFile();
        json json_data = createJsonObjectRule();
        // create json data for rule without comments.
        json_data["rules"][0].erase("comments");

        std::string string_json = json_data.dump();
        std::ofstream out(file_name_);
        out << string_json;
        out.close();

        EXPECT_FILE_VALID(file_name_);
        unlink(file_name_.c_str());
    }

    //invalid test comments property has invalid value type
    {
        file_name_ = createTmpFile();
        json json_data = createJsonObjectRule();
        // create json data
        json_data["rules"][0]["comments"] = {1};

        std::string string_json = json_data.dump();
        std::ofstream out(file_name_);
        out << string_json;
        out.close();

        EXPECT_FILE_INVALID(file_name_, \
                            "Schema validation failed\n", \
                            "1 is not of type u'string'\n");
        unlink(file_name_.c_str());
    }

    // invalid test rule with no ID
    {

        file_name_ = createTmpFile();
        json json_data = createJsonObjectRule();
        // create json data for rule without ID.
        json_data["rules"][0].erase("id");

        std::string string_json = json_data.dump();
        std::ofstream out(file_name_);
        out << string_json;
        out.close();

        EXPECT_FILE_INVALID(file_name_, \
                            "Schema validation failed\n", \
                            "u'id' is a required property\n");
        unlink(file_name_.c_str());
    }

    // invalid test id property has invalid value type (not strings)
    {

        file_name_ = createTmpFile();
        json json_data = createJsonObjectRule();
        // create json data with id = true
        json_data["rules"][0]["id"] = true;

        std::string string_json = json_data.dump();
        std::ofstream out(file_name_);
        out << string_json;
        out.close();

        EXPECT_FILE_INVALID(file_name_, \
                            "Schema validation failed\n", \
                            "True is not of type u'string'\n");
        unlink(file_name_.c_str());
    }

    // invalid test id property has invalid value
    {

        file_name_ = createTmpFile();
        json json_data = createJsonObjectRule();
        // create json data with id = foo%
        json_data["rules"][0]["id"] = "foo%";

        std::string string_json = json_data.dump();
        std::ofstream out(file_name_);
        out << string_json;
        out.close();

        EXPECT_FILE_INVALID(file_name_, \
                            "Schema validation failed\n", \
                            "u'foo%' does not match u'^[A-Za-z0-9_]+$'\n");
        unlink(file_name_.c_str());
    }

    // invalid test rule with no action
    {

        file_name_ = createTmpFile();
        json json_data = createJsonObjectRule();
        // create json data for rule without actions
        json_data["rules"][0].erase("actions");

        std::string string_json = json_data.dump();
        std::ofstream out(file_name_);
        out << string_json;
        out.close();

        EXPECT_FILE_INVALID(file_name_, \
                            "Schema validation failed\n", \
                            "u'actions' is a required property\n");
        unlink(file_name_.c_str());
    }

    // valid test rule with multiple actions
    {

        file_name_ = createTmpFile();
        json json_data = createJsonObjectRule();
        // create json data with multiple actions
        json_data["rules"][0]["actions"][1]["run_rule"] = "set_page0_voltage_rule";

        std::string string_json = json_data.dump();
        std::ofstream out(file_name_);
        out << string_json;
        out.close();

        EXPECT_FILE_VALID(file_name_);
        unlink(file_name_.c_str());
    }

    // invalid test action property has invalid value type(not an array)
    {

        file_name_ = createTmpFile();
        json json_data = createJsonObjectRule();
        // create json data with action = 1
        json_data["rules"][0]["actions"] = 1;

        std::string string_json = json_data.dump();
        std::ofstream out(file_name_);
        out << string_json;
        out.close();

        EXPECT_FILE_INVALID(file_name_, \
                            "Schema validation failed\n", \
                            "1 is not of type u'array'\n");
        unlink(file_name_.c_str());
    }

    // invalid test action property has invalid value of action
    {

        file_name_ = createTmpFile();
        json json_data = createJsonObjectRule();
        // create json data with action[0] = foo
        json_data["rules"][0]["actions"][0] = "foo";

        std::string string_json = json_data.dump();
        std::ofstream out(file_name_);
        out << string_json;
        out.close();

        EXPECT_FILE_INVALID(file_name_, \
                            "Schema validation failed\n", \
                            "u'foo' is not of type u'object'\n");
        unlink(file_name_.c_str());
    }

}
