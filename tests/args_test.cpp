#include <gtest/gtest.h>
#include <map>
#include <string>
#include <vector>

static std::map<std::string, std::string> parseArgs(std::vector<std::string> const& args)
{
    std::map<std::string, std::string> values;

    for (size_t i = 1; i + 1 < args.size(); ++i) {
        if (args[i].rfind("--", 0) == 0) {
            values[args[i].substr(2)] = args[i + 1];
            ++i;
        }
    }

    return values;
}

TEST(Args, ParsesKeyValueOptions)
{
    const std::vector<std::string> args = {"app", "--mode", "test", "--path", "/tmp/log"};
    const auto values = parseArgs(args);
    EXPECT_EQ(values.at("mode"), "test");
    EXPECT_EQ(values.at("path"), "/tmp/log");
}
