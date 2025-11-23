#include "headers.hpp"






Router::Router(const std::string &docroot, const std::string &uploadsDir, const std::string &index, const Server_struct &serverConfig)
    : m_docroot(docroot),
      m_uploads(uploadsDir),
      m_index(index),
      m_serverConfig(serverConfig)
{
}

std::string Request::getQuery() const {
    size_t pos = m_path.find('?');
    if (pos == std::string::npos)
        return "";
    return m_path.substr(pos + 1);
}


bool Router::isMethodAllowed(const std::string &method, const std::string &path)
{
    if (path == "/static" || path.find("/static/") == 0 || path == "/index.html")
        return method == "GET";


    if (path == "/upload" || path == "/upload.html" ||
        (path.find("/upload/") == 0 && path.find("/uploads/") != 0))
        return method == "GET";

 
    if (path == "/uploads" || path.find("/uploads/") == 0)
        return method == "GET" || method == "POST" || method == "DELETE";


    return method == "GET";
}

std::string Router::getContentType(const std::filesystem::path &filePath)
{
    if (filePath.empty())
        return "application/octet-stream";

    std::string ext = filePath.extension().string();
    if (ext.empty())
        return "application/octet-stream";

    ext = stringToLower(ext);

    if (ext == ".html" || ext == ".htm")  return "text/html; charset=UTF-8";
    if (ext == ".css")  return "text/css; charset=UTF-8";
    if (ext == ".js")  return "application/javascript; charset=UTF-8";
    if (ext == ".txt") return "text/plain; charset=UTF-8";
    if (ext == ".jpg" || ext == ".jpeg")  return "image/jpeg";
    if (ext == ".png") return "image/png";
    if (ext == ".gif") return "image/gif";
    if (ext == ".ico") return "image/x-icon";
    if (ext == ".pdf") return "application/pdf";

    return "application/octet-stream";
}


Response Router::handleRequest(const Request &req)
{
    std::string method = req.getMethod();
    std::string path = req.getPath();

    std::string normalizedPath = path;

    Response res;

    if (normalizedPath == "/old-page")
    {
        res.setStatus(301, "Moved Permanently");
        res.setHeader("Location", "/");
        res.setHeader("Content-Length", "0");
        res.setBody("");
        res.initialize(res.getStatusCode(), res.getStatusMessage(), req);
        return res;
    }
    
    if (normalizedPath == "/redirect-upload")
    {
        res.setStatus(302, "Found");
        res.setHeader("Location", "/upload");
        res.setHeader("Content-Length", "0");
        res.setBody("");
        res.initialize(res.getStatusCode(), res.getStatusMessage(), req);
        return res;
    }
    
    if (normalizedPath == "/redirect-calculator")
    {
        res.setStatus(307, "Temporary Redirect");
        res.setHeader("Location", "/calculator");
        res.setHeader("Content-Length", "0");
        res.setBody("");
        res.initialize(res.getStatusCode(), res.getStatusMessage(), req);
        return res;
    }

    if (!isMethodAllowed(method, normalizedPath))
        res = Response::withStatus(405);
    else if (method == "GET")
        res = handleGET(normalizedPath);
    else if (method == "POST")
        res = handlePOST(req);
    else if (method == "DELETE")
        res = handleDELETE(normalizedPath);
    else
        res = Response::withStatus(405);

    res.initialize(res.getStatusCode(), res.getStatusMessage(), req);

    return res;
}

