#include "headers.hpp"

const std::string Response::getDefaultMessage(unsigned int statusCode)
{
	std::map<unsigned int, std::string> statusMessages =
	{{200, "OK"},
    {201, "Created"},
    {301, "Moved Permanently"},
    {302, "Found"},
    {307, "Temporary Redirect"},
    {308, "Permanent Redirect"},
    {400, "Bad Request"},
    {403, "Forbidden"},
    {404, "Not Found"},
    {405, "Method Not Allowed"},
    {408, "Request Timeout"},
    {409, "Conflict"},
    {411, "Length Required"},
    {413, "Payload Too Large"},
    {414, "Request-URI Too Long"},
    {415, "Unsupported Media Type"},
    {417, "Expectation Failed"},
    {431, "Request Header Fields Too Large"},
    {500, "Internal Server Error"},
    {501, "Not Implemented"},
    {502, "Bad Gateway"},
    {503, "Service Unavailable"},
    {504, "Gateway Timeout"},
    {505, "HTTP Version Not Supported"}};

	for (auto it : statusMessages)
		if (it.first == statusCode)
			return it.second;

	return "Unknown Error";
}

static bool shouldKeepAlive(const std::string& version, const std::string& connection)
{
    if (version == "HTTP/1.1")
        return connection != "close";
    if (version == "HTTP/1.0")
        return connection == "keep-alive";
    return false;
}

void Response::initialize(unsigned statusCode, const std::string& message, const Request& request)
{
	this->setStatus(statusCode, message);
	this->m_version = request.getVersion();

	this->setMeta(request.getVersion(), request.getHeader("Connection"));

    const std::string& connection = request.getHeader("Connection");
    const std::string& version = request.getVersion();

    const std::set<int> mustClose = {400, 408, 413, 500};
    bool keepAlive = (mustClose.count(statusCode) == 0) && shouldKeepAlive(version, connection);

    this->setHeader("Connection", keepAlive ? "keep-alive" : "close");
}

Response Response::withStatus(int errorNumber)
{
	Response res;

	res.setBody("<html><head><title>"+ std::to_string(errorNumber) +"</title></head><body>"+ getDefaultMessage(errorNumber) + "</body></html>");
	res.setStatus(errorNumber, getDefaultMessage(errorNumber));
	res.setHeader("Content-Type", "text/html");
	res.setHeader("Content-Length", std::to_string(res.getBody().size()));

	if (errorNumber == 204)
	{
		res.setBody("");
		res.setHeader("Content-Length", "0");
	}

	return res;
}
