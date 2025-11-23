#include "headers.hpp"

std::string Request::getConnectionType() const
{
    std::string connection = getHeader("Connection");

    if (connection.empty())
    {
        if (m_httpVersion == "HTTP/1.0")
            return "close";
        return "";
    }

    return stringToLower(connection);
}

std::string methodChecker(const std::string &method)
{
    std::string methods[3] = {"DELETE", "GET", "POST"};

    for (const auto &m : methods)
        if (method == m)
            return m;
    return "";
}

static std::pair<std::string, std::string> parseHeaderline(const std::string &line)
{
    size_t colon = line.find(':');

    if (colon == std::string::npos)
        return {"", ""};

    std::string key = line.substr(0, colon);

    size_t start = colon + 1;
    while (start < line.length() && (line[start] == ' ' || line[start] == '\t'))
        start++;
    std::string value = line.substr(start);

    while (!value.empty() && (value.back() == ' ' || value.back() == '\t' || value.back() == '\r'))
        value.pop_back();

    return {key, value};
}

static void parseRequestLine(const std::string &line, Request &req)
{
    if (line.empty())
        throw std::invalid_argument("Request line is empy");

    std::istringstream iss(line);
    std::string method, path, version;

    iss >> method >> path >> version;
    if (method.empty() || path.empty() || version.empty())
        throw std::invalid_argument("Missing argument on request line");

    method = stringToUpper(method);
    std::string validMethod = methodChecker(method);
    if (validMethod.empty())
        throw std::invalid_argument("No valid method provided! {GET, POST, DELETE}");
    req.setMethod(method);
    req.setPath(path);
    req.setVersion(version);
}

static std::string bodyContentExtractor(const std::string &content, const std::string &boundary)
{
    if (content.empty() || boundary.empty())
        throw std::invalid_argument("Body content missing");

    std::string contentDisposition = "";
    size_t start = content.find("Content-Disposition");
    size_t end = content.find("\r\n", start);

    if (start != std::string::npos && end != std::string::npos)
        contentDisposition = content.substr(start + 19, end - start - 19);

    std::string filename = extractFilename(contentDisposition);

    start = content.find("\r\n\r\n");
    if (start != std::string::npos)
        start += 4;
    end = content.find("--" + boundary, start);
    if (end == std::string::npos)
        return "";
    std::string body = content.substr(start, end - start);
    while (!body.empty() && (body.back() == '\n' || body.back() == '\r'))
        body.pop_back();

    return body;
}

Request RequestParser::parse(const std::string &rawRequest)
{
    if (rawRequest.empty())
        throw std::invalid_argument("Raw request missing for parse Request");

    Request req;
    std::istringstream iss(rawRequest);
    std::string line = "";
    std::string boundary = "";

    if (std::getline(iss, line))
    {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        parseRequestLine(line, req);
    }
    while (std::getline(iss, line))
    {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (line.empty())
            break;
        auto [key, value] = parseHeaderline(line);
        if (!key.empty())
            req.setHeader(key, value);
        if (stringToLower(key) == "host")
            req.setHost(value);

        size_t pos = line.find("boundary=");
        if (pos != std::string::npos)
        {
            std::string b = line.substr(pos + 9);
            if (!b.empty() && b.front() == '"')
                b.erase(0, 1);
            if (!b.empty() && b.back() == '"')
                b.pop_back();
            boundary = b;
        }
    }

    std::string body((std::istreambuf_iterator<char>(iss)), {});
    req.setRawBody(body); 

    if (body.empty())
    {
        req.setBody("");
        return req;
    }


    std::string transferEncoding = stringToLower(req.getHeader("Transfer-Encoding"));
    if (transferEncoding == "chunked")
    {
        req.setBody("");
        return req;
    }


    std::string ctype = stringToLower(req.getHeader("Content-Type"));
    if (ctype.find("multipart/form-data") != std::string::npos && !boundary.empty())
        req.setBody(bodyContentExtractor(body, boundary));
    else
        req.setBody(body);

    return req;
}
