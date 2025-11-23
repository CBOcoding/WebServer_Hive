#pragma once
#include <map>
#include <string>

struct CgiResult {
    int status = 200;
    std::map<std::string, std::string> headers;
    std::string body;
};


class Request;

CgiResult runCgi(const Request& req,
                 const std::string& scriptPath,
                 const std::string& interpreter);
