#include "headers.hpp"

Response::Response()
    : m_statusCode(200),
      m_statusMessage("OK"),
      m_version("HTTP/1.1"),
      m_connection("close") {}

Response::~Response()
{
}

// setter

void Response::setStatus(int code, const std::string &message)
{
	m_statusCode = code;
	m_statusMessage = message;
}
void Response::setHeader(const std::string &key, const std::string &value)
{
	m_headers[key] = value;
}
void Response::setFilePath(const std::string &path)
{
	m_filePath = path;
}
void Response::setBody(const std::string &body)
{
	m_body = body;
}

void Response::setMeta(const std::string &version, const std::string &connection)
{
	m_version = version;
	m_connection = connection;
}

// getter
int Response::getStatusCode() const
{
	return m_statusCode;
}
const std::string &Response::getStatusMessage() const
{
	return m_statusMessage;
}
const std::string &Response::getFilePath() const
{
	return m_filePath;
}

const std::map<std::string, std::string> &Response::getHeaders(void) const
{
	return m_headers;
}
const std::string &Response::getBody() const
{
	return m_body;
}

void Response::loadFile()
{
    if (!std::filesystem::exists(m_filePath)) {
        setStatus(404, "Not Found");
        m_body = "<html><body><h1>404 Not Found</h1></body></html>";
        return;
    }

    std::ifstream file(m_filePath, std::ios::binary);
    if (!file.is_open()) {
        setStatus(500, "Internal Server Error");
        m_body = "<html><body><h1>500 Internal Server Error</h1></body></html>";
        return;
    }

    std::ostringstream outStream;
    outStream << file.rdbuf();
    m_body = outStream.str();

    if (getStatusCode() == 0)
        setStatus(200, "OK");

    setHeader("Content-Length", std::to_string(m_body.size()));
    if (m_headers.find("Content-Type") == m_headers.end())
        setHeader("Content-Type", "text/html");

    setHeader("Connection", "close");
}


std::string Response::serializer()
{
    std::ostringstream out;

  
    out << m_version << " "
        << getStatusCode() << " "
        << getStatusMessage() << "\r\n";


    if (m_headers.find("Content-Length") == m_headers.end())
        m_headers["Content-Length"] = std::to_string(m_body.size());

    if (m_headers.find("Content-Type") == m_headers.end())
        m_headers["Content-Type"] = "text/html";

    if (m_headers.find("Connection") == m_headers.end())
        m_headers["Connection"] = "close";

   
    for (const auto &[key, value] : m_headers)
        out << key << ": " << value << "\r\n";

    out << "\r\n"; 
    out << m_body; 

    return out.str();
}

Response Response::fromErrorCode(int code, const Server_struct &server)
{
    Response res;
    res.setStatus(code, getDefaultMessage(code));

    std::string errorPage;
    bool useCustomPage = false;

    if (server.error_pages.find(code) != server.error_pages.end())
    {
        errorPage = server.error_pages.at(code);
        std::ifstream file(errorPage.c_str());
        if (file.is_open())
        {
            std::stringstream buffer;
            buffer << file.rdbuf();
            res.setBody(buffer.str());
            file.close();
            useCustomPage = true;
        }
    }

    if (!useCustomPage)
    {
        std::string defaultErrorPage = server.root + "/error.html";
        std::ifstream file(defaultErrorPage.c_str());
        if (file.is_open())
        {
            std::stringstream buffer;
            buffer << file.rdbuf();
            res.setBody(buffer.str());
            file.close();
            useCustomPage = true;
        }
    }

    if (!useCustomPage)
    {
        res.setBody("<html><head><title>" + std::to_string(code) + " " + 
                    getDefaultMessage(code) + "</title></head><body><h1>" + 
                    std::to_string(code) + " " + getDefaultMessage(code) + 
                    "</h1></body></html>");
    }

    res.setHeader("Content-Type", "text/html");
    res.setHeader("Content-Length", std::to_string(res.getBody().size()));

    return res;
}