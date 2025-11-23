#include "headers.hpp"

std::string stringToUpper(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c)
                   { return std::toupper(c); });
    return s;
}

std::string stringToLower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c)
                   { return std::tolower(c); });
    return s;
}

std::string extractFilename(const std::string &contentDisposition)
{
    if (contentDisposition.empty())
        return "";

    std::string content = stringToLower(contentDisposition);

    size_t start = content.find("filename=");
    if (start == std::string::npos)
        return "";

    start += 9;

    while (start < content.size() && (isspace(content[start]) || content[start] == '"'))
        start++;

    size_t end = content.find('"', start);
    if (end == std::string::npos)
        end = content.find(';', start);

    if (end == std::string::npos)
        end = content.size();

    return content.substr(start, end - start);
}