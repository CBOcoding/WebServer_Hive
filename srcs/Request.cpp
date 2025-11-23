#include "headers.hpp"

// Getters

const std::string &Request::getMethod() const
{
    return m_method;
}

const std::string &Request::getPath() const
{
    return m_path;
}

const std::string &Request::getVersion() const
{
    return m_httpVersion;
}

const std::string Request::getHeader(const std::string &key) const
{
    auto it = m_headers.find(key);
    return (it != m_headers.end()) ? it->second : "";
}

const std::map<std::string, std::string> &Request::getHeaders() const
{
    return m_headers;
}
const std::string &Request::getBody() const
{
    return m_body;
}

const std::string &Request::getHost() const
{
    return m_host;
}

const std::string &Request::getRawBody() const
{
    return m_rawBody;
}

// Setter

void Request::setMethod(const std::string &method)
{
    m_method = method;
}
void Request::setPath(const std::string &path)
{
    m_path = path;
}
void Request::setVersion(const std::string &httpVersion)
{
    m_httpVersion = httpVersion;
}
void Request::setHeader(const std::string &key, const std::string &value)
{
    m_headers[key] = value;
}

void Request::setBody(const std::string &body)
{
    m_body = body;
}

void Request::setHost(const std::string &host)
{
    m_host = host;
}

void Request::setRawBody(const std::string &rawBody)
{
    m_rawBody = rawBody;
}
