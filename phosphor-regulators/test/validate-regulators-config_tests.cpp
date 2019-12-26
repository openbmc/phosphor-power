/**
 * Copyright © 2019 IBM Corporation
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

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#define EXPECT_FILE_VALID(tmpFile) expect_file_valid(tmpFile)
#define EXPECT_FILE_INVALID(tmpFile, expectedErrorMessage, expectedOutputMessage) expect_file_invalid(tmpFile, expectedErrorMessage, expectedOutputMessage)

using json = nlohmann::json;

std::string create_tmp_file()
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

std::string validate_regulators_config_command(const std::string& config_file_name)
{
    std::string python_command = "python ";
    // validate-regulators-config.py path
    std::string pythontool = "../tools/validate-regulators-config.py ";
    // validate-regulators-config.py command args and config_schema.json path
    std::string schema_args = "-s ";
    std::string schema_file = " ../schema/config_schema.json ";
    std::string config_file_args = "-c ";
    std::string command ="";

    command += python_command;
    command += pythontool;
    command += schema_args;
    command += schema_file;
    command += config_file_args;
    command += config_file_name;

    return command;
}

std::string run_tool_stderr(const std::string& config_file_name)
{
    // run the python tool(validate-regulators-config.py) with the temporary file and return the output of the python tool.
    std::string temp;
    std::string command = validate_regulators_config_command(config_file_name);

    command += " 2>&1 >/dev/null"; // only get the stderr from validate-regulators-config.py.

    char buffer[128];
    std::string result = "";
    // to get the stderr from the validate-regulators-config pyhton tool.
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) throw std::runtime_error("popen() failed!");
    if (fgets(buffer, sizeof buffer, pipe) != NULL)
    {
        result = buffer;
    }
    pclose(pipe);
    return result;
}

std::string run_tool_stdout(const std::string& config_file_name)
{
    // run the python tool(validate-regulators-config.py) with the temporary file and return the output of the python tool.
    std::string temp;
    std::string command = validate_regulators_config_command(config_file_name);

    // get the jsonschema print from python tool.
    char buffer[256];
    std::string result = "";
    // to get the stdout from the validate-regulators-config pyhton tool.
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) throw std::runtime_error("popen() failed!");
    if (fgets(buffer, sizeof buffer, pipe) != NULL)
    {
        result = buffer;
    }
    pclose(pipe);
    return result;
}

int run_tool_exit_code(const std::string& config_file_name)
{
    // run the python tool(validate-regulators-config.py) with the temporary file and return the output of the python tool.
    std::string temp;
    std::string command = validate_regulators_config_command(config_file_name);

    // to get the exit code for the validate-regulators-config python tool.
    int exitCode = system(command.c_str());
    return exitCode;
}

void expect_file_valid(const std::string& tmpFile)
{
    EXPECT_EQ(run_tool_exit_code(tmpFile), 0);
    EXPECT_EQ(run_tool_stderr(tmpFile), "");
    EXPECT_EQ(run_tool_stdout(tmpFile), "");
}

void expect_file_invalid(const std::string& tmpFile, const std::string& expectedErrorMessage, const std::string& expectedOutputMessage)
{
    EXPECT_NE(run_tool_exit_code(tmpFile), 0);
    EXPECT_EQ(run_tool_stderr(tmpFile), expectedErrorMessage);
    EXPECT_EQ(run_tool_stdout(tmpFile), expectedOutputMessage);
}

// create json data for "rule" object
json create_json_object_rule()
{
    json json_data;

    // create json data for rule
    json_data["comments"] = {"Config file for a FooBar one-chassis system"};
    json_data["rules"][0]["comments"] = {"Sets output voltage for a PMBus regulator rail"};
    json_data["rules"][0]["id"] = "set_voltage_rule";
    json_data["rules"][0]["actions"][0]["pmbus_write_vout_command"]["format"] = "linear";

    return json_data;
}

TEST(ValidateRegulatorsConfigTest, Rule)
{

    std::string file_name_ ;

    // valid test comments property, one id property, one action property specified.
    {

        file_name_ = create_tmp_file();
        json json_data = create_json_object_rule();
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

        file_name_ = create_tmp_file();
        json json_data = create_json_object_rule();
        // create json data for rule without comments.
        json_data["rules"][0].erase("comments");

        std::string string_json = json_data.dump();
        std::ofstream out(file_name_);
        out << string_json;
        out.close();

        EXPECT_FILE_VALID(file_name_);
        unlink(file_name_.c_str());
    }

    //invalid test comments property has invalid value type (not an array of strings)
    {
        file_name_ = create_tmp_file();
        json json_data = create_json_object_rule();
        // create json data
        json_data["rules"][0]["comments"] = {1};

        std::string string_json = json_data.dump();
        std::ofstream out(file_name_);
        out << string_json;
        out.close();

        EXPECT_FILE_INVALID(file_name_, "Schema validation failed\n","1 is not of type u'string'\n");
        unlink(file_name_.c_str());
    }

    // invalid test rule with no ID
    {

        file_name_ = create_tmp_file();
        json json_data = create_json_object_rule();
        // create json data for rule without ID.
        json_data["rules"][0].erase("id");

        std::string string_json = json_data.dump();
        std::ofstream out(file_name_);
        out << string_json;
        out.close();

        EXPECT_FILE_INVALID(file_name_, "Schema validation failed\n","u'id' is a required property\n");
        unlink(file_name_.c_str());
    }

    // invalid test id property has invalid value type (not strings)
    {

        file_name_ = create_tmp_file();
        json json_data = create_json_object_rule();
        // create json data with id = true
        json_data["rules"][0]["id"] = true;

        std::string string_json = json_data.dump();
        std::ofstream out(file_name_);
        out << string_json;
        out.close();

        EXPECT_FILE_INVALID(file_name_, "Schema validation failed\n","True is not of type u'string'\n");
        unlink(file_name_.c_str());
    }

    // invalid test id property has invalid value
    {

        file_name_ = create_tmp_file();
        json json_data = create_json_object_rule();
        // create json data with id = foo%
        json_data["rules"][0]["id"] = "foo%";

        std::string string_json = json_data.dump();
        std::ofstream out(file_name_);
        out << string_json;
        out.close();

        EXPECT_FILE_INVALID(file_name_, "Schema validation failed\n","u'foo%' does not match u'^[A-Za-z0-9_]*$'\n");
        unlink(file_name_.c_str());
    }

    // invalid test rule with no action
    {

        file_name_ = create_tmp_file();
        json json_data = create_json_object_rule();
        // create json data for rule without actions
        json_data["rules"][0].erase("actions");

        std::string string_json = json_data.dump();
        std::ofstream out(file_name_);
        out << string_json;
        out.close();

        EXPECT_FILE_INVALID(file_name_, "Schema validation failed\n","u'actions' is a required property\n");
        unlink(file_name_.c_str());
    }

    // valid test rule with multiple actions
    {

        file_name_ = create_tmp_file();
        json json_data = create_json_object_rule();
        // create json data with multiple actions
        json_data["rules"][0]["actions"][1]["run_rule"] = "run_rule"; //string

        std::string string_json = json_data.dump();
        std::ofstream out(file_name_);
        out << string_json;
        out.close();

        EXPECT_FILE_VALID(file_name_);
        unlink(file_name_.c_str());
    }

    // invalid test action property has invalid value type(not an array)
    {

        file_name_ = create_tmp_file();
        json json_data = create_json_object_rule();
        // create json data with action = 1
        json_data["rules"][0]["actions"] = 1;

        std::string string_json = json_data.dump();
        std::ofstream out(file_name_);
        out << string_json;
        out.close();

        EXPECT_FILE_INVALID(file_name_, "Schema validation failed\n","1 is not of type u'array'\n");
        unlink(file_name_.c_str());
    }

    // invalid test action property has invalid value of action
    {

        file_name_ = create_tmp_file();
        json json_data = create_json_object_rule();
        // create json data with action[0] = foo
        json_data["rules"][0]["actions"][0] = "foo";

        std::string string_json = json_data.dump();
        std::ofstream out(file_name_);
        out << string_json;
        out.close();

        EXPECT_FILE_INVALID(file_name_, "Schema validation failed\n","u'foo' is not of type u'object'\n");
        unlink(file_name_.c_str());
    }

    // invalid test rule action with no property invalid
    {

        file_name_ = create_tmp_file();
        json json_data = create_json_object_rule();
        // create json data for rule actions without property.
        json_data["rules"][0].erase("actions");
        json_data["rules"][0]["actions"][0]["comments"] = {"Read output voltage from READ_VOUT."};

        std::string string_json = json_data.dump();
        std::ofstream out(file_name_);
        out << string_json;
        out.close();

        EXPECT_FILE_INVALID(file_name_, "Schema validation failed\n","u'and' is a required property\n");
        unlink(file_name_.c_str());
    }

    // invalid test rule action with two property invalid
    {

        file_name_ = create_tmp_file();
        json json_data = create_json_object_rule();
        // create json data for rule actions with two property invalid.
        json_data["rules"][0]["actions"][0]["run_rule"] = "run_rule"; //string

        std::string string_json = json_data.dump();
        std::ofstream out(file_name_);
        out << string_json;
        out.close();

        EXPECT_FILE_INVALID(file_name_, "Schema validation failed\n","{u'pmbus_write_vout_command': {u'format': u'linear'}, u'run_rule': u'run_rule'} is valid under each of {u'required': [u'run_rule']}, {u'required': [u'pmbus_write_vout_command']}\n");
        unlink(file_name_.c_str());
    }

}
