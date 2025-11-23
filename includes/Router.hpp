#pragma once

#include "headers.hpp"

class Response;

class Router
{
    private:
    std::string m_docroot;
    std::string m_uploads;
    std::string m_index;
    Server_struct m_serverConfig;
    
    Response handleGET(const std::string& path);
    Response handlePOST(const Request& req);
    Response handleDELETE(const std::string& path);
    std::string getContentType(const std::filesystem::path& filePath);

	Response generateDirectoryListing(const std::string &fullPath, const std::string &urlPath);
    std::string formatFileSize(std::uintmax_t bytes);

    public:
    Router() = default;
    ~Router() = default;
    Router(const std::string &docroot, const std::string &uploadsDir, const std::string &index, const Server_struct &serverConfig);

    Response handleRequest(const Request& req);
    Response create405Response();
    bool isMethodAllowed(const std::string& method, const std::string& path);
};