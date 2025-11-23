#pragma once

#include"headers.hpp"

class Request;
class Router;

class RequestParser
{
private:
public:

	RequestParser()  = default;
	~RequestParser()  = default;                            
	RequestParser(const RequestParser& other) = default;            
	RequestParser& operator=(const RequestParser& other) = default;
	
	Request parse(const std::string& rawRequest);

};


