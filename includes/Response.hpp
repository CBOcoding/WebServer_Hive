#pragma once

#include "headers.hpp"

class Request;

class Response
{
private:
	unsigned int m_statusCode;
	std::string m_statusMessage;
	std::string m_filePath;
	std::map<std::string, std::string> m_headers;
	std::string m_body;
	std::string m_version;
	std::string m_connection;

public:
	Response();
	Response(const Response &) = default;
	Response &operator=(const Response &) = default;
	~Response();

	// setter

	void setStatus(int code, const std::string &message);
	void setHeader(const std::string &key, const std::string &value);
	void setFilePath(const std::string &path);
	void setBody(const std::string &body);
	void setMeta(const std::string &version, const std::string &connection);

	// getter
	int getStatusCode() const;
	const std::string &getStatusMessage() const;
	const std::string &getFilePath() const;
	const std::map<std::string, std::string> &getHeaders(void) const;
	const std::string &getBody() const;

	static const std::string getDefaultMessage(unsigned int statusCode);

	std::string serializer();
	void loadFile();
	void initialize(unsigned statusCode, const std::string &message, const Request &request);

	static Response withStatus(int erroNumber);
    static Response fromErrorCode(int code, const Server_struct &server);
};
