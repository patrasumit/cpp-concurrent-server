#include "http/HttpUtils.h"

#include <algorithm>
#include <cctype>

std::string toLower(const std::string& input)
{
    std::string result = input;

    std::transform(
        result.begin(),
        result.end(),
        result.begin(),
        [](unsigned char c)
        {
            return std::tolower(c);
        }
    );

    return result;
}