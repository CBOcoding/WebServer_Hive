#pragma once
#include "headers.hpp"

class Request
{
private:
	std::string m_method;
	std::string m_path;
	std::string m_httpVersion;
	std::map<std::string, std::string> m_headers;
	std::string m_body;
	std::string m_host;
    std::string m_rawBody;

public:
	Request() = default;
	~Request() = default;
	Request(const Request &other) = default;
	Request &operator=(const Request &other) = default;

	// Getter

	const std::string &getMethod() const;
	const std::string &getPath() const;
	const std::string &getVersion() const;
	const std::string getHeader(const std::string &key) const;
	const std::map<std::string, std::string> &getHeaders() const;
	const std::string &getBody() const;
	const std::string &getHost() const;
    const std::string& getRawBody() const;
	std::string getQuery() const;
	std::string getConnectionType() const;


	// Setter

	void setMethod(const std::string &method);
	void setPath(const std::string &path);
	void setVersion(const std::string &httpVersion);
	void setHeader(const std::string &key, const std::string &value);
	void setBody(const std::string &body);
	void setHost(const std::string &body);
    void setRawBody(const std::string &rawBody);

};
