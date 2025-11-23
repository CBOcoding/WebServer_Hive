#pragma once

#include "headers.hpp"

class Request;
class Server
{
private:
    Config_struct m_config;
    std::unordered_map<int, int> m_clientOrigin;
    std::unordered_map<int, std::string> m_inbuf;
    std::map<int, std::string> m_outbuf;
    std::unordered_set<int> m_listenerFdSet;
    std::unordered_map<uint16_t, int> m_portToFd;
    std::unordered_map<int, uint16_t> m_fdToPort;
    std::vector<struct pollfd> m_fds;
    std::unordered_set<uint16_t> m_seen;

    void closeClientConnection(int client_fd, size_t &index);
    void initializeListeners();
    void handlePollError(struct pollfd &pfd, size_t &index);
    void acceptNewConnections(int listenerFd);
    void handleClientWrite(struct pollfd &pfd, size_t &index);
    bool readClientData(int client_fd);
    void setLocationDefaults(const Server_struct &server,
                             const Location_struct *location,
                             std::string &docroot,
                             std::string &uploadDir,
                             std::string &indexName);
    bool checkCgiRequest(const Request &request,
                         const Location_struct *location,
                         std::string &cgiExtension,
                         std::string &cgiInterpreterPath);
    std::string buildScriptPath(const Request &request,
                                const Location_struct *location,
                                const std::string &docroot,
                                const std::string &indexName);
    bool validateScriptPath(const std::string &scriptPath, const std::string &docroot);
    void handleCgiRequest(int client_fd,
                          const Request &request,
                          const std::string &scriptPath,
                          const std::string &cgiInterpreterPath);
    void processCompleteRequest(int client_fd, size_t &index);
    void handleClientRead(struct pollfd &pfd, size_t &index);

public:
    Server();
    ~Server() = default;
    Server(const Server &other) = default;
    Server &operator=(const Server &other) = default;

    Server(const Config_struct &cfg);

    int start_server(void);
    std::string readFile(const std::string &filename);
    void sendError(int client_fd, int code, const Server_struct &server);
};
