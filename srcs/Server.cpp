#include "headers.hpp"

Server::Server(const Config_struct &cfg) : m_config(cfg) {}
Server::Server() {}

static const Location_struct *matchLocation(const Server_struct &server, const std::string &path)
{
	const Location_struct *bestMatch = NULL;
	size_t bestLen = 0;

	for (size_t i = 0; i < server.locations.size(); ++i)
	{
		const Location_struct &loc = server.locations[i];
		if (path.rfind(loc.path, 0) == 0)
		{
			if (loc.path.size() > bestLen)
			{
				bestMatch = &loc;
				bestLen = loc.path.size();
			}
		}
	}
	return bestMatch;
}

static int create_listener(uint16_t port)
{
	int listenerFd = ::socket(AF_INET, SOCK_STREAM, 0);
	if (listenerFd < 0)
	{
		perror("socket");
		std::exit(1);
	}

	int opt = 1;
	if (::setsockopt(listenerFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
	{
		perror("setsockopt");
		std::exit(1);
	}

	int flags = ::fcntl(listenerFd, F_GETFL, 0);
	if (flags < 0 || ::fcntl(listenerFd, F_SETFL, flags | O_NONBLOCK) < 0)
	{
		perror("fcntl");
		std::exit(1);
	}

	struct sockaddr_in addr;
	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;

	if (::bind(listenerFd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		perror("bind");
		std::exit(1);
	}

	if (::listen(listenerFd, 128) < 0)
	{
		perror("listen");
		std::exit(1);
	}

	return listenerFd;
}

static const Server_struct *findServerByPort(const Config_struct &config, uint16_t port)
{
	for (const auto &srv : config.servers)
	{
		if (srv.listen_port == port)
			return &srv;
	}
	return &config.servers.front();
}

std::string Server::readFile(const std::string &filename)
{
	std::ifstream file(filename.c_str());
	if (!file.is_open())
	{
		std::cerr << "Failed to open file: " << filename << std::endl;
		return "";
	}
	std::stringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}

static std::string GetHeaderValueCI(const std::string &headers, const std::string &name)
{
	std::istringstream iss(headers);
	std::string line;
	std::string low = name;

	while (std::getline(iss, line))
	{
		if (line == "\r")
			break;

		size_t colon = line.find(':');
		if (colon == std::string::npos)
			continue;

		std::string key = line.substr(0, colon);
		for (size_t i = 0; i < key.size(); ++i)
			key[i] = std::tolower(key[i]);

		if (key == low)
		{
			std::string val = line.substr(colon + 1);
			size_t startIndexOfLength = val.find_first_not_of(" \t");
			size_t endIndexOfLength = val.find_last_not_of(" \t\r\n");

			if (startIndexOfLength == std::string::npos)
				return "";
			else
				return val.substr(startIndexOfLength, endIndexOfLength - startIndexOfLength + 1);
		}
	}
	return "";
}

static bool IsChunkedRequestComplete(const std::string &inbuf, size_t bodyStartPos)
{
	const std::string body = inbuf.substr(bodyStartPos);
	return (body.find("0\r\n\r\n") != std::string::npos);
}

static bool HTTP_IsRequestComplete(const std::string &inbuf)
{
	size_t hdrEndPos = inbuf.find("\r\n\r\n");
	if (hdrEndPos == std::string::npos)
		return false;

	const std::string allHeaders = inbuf.substr(0, hdrEndPos + 4);
	const std::string contentLengthValue = GetHeaderValueCI(allHeaders, "content-length");

	if (contentLengthValue.empty())
	{
		const std::string transferEncodingValue = GetHeaderValueCI(allHeaders, "transfer-encoding");
		if (transferEncodingValue == "chunked")
			return (IsChunkedRequestComplete(inbuf, hdrEndPos + 4));
		return true;
	}

	size_t needBody = static_cast<size_t>(std::strtoul(contentLengthValue.c_str(), NULL, 10));
	size_t haveBody;
	const size_t END_OF_HEADERS = hdrEndPos + 4;

	if (inbuf.size() >= END_OF_HEADERS)
		haveBody = inbuf.size() - END_OF_HEADERS;
	else
		haveBody = 0;

	return (haveBody >= needBody);
}

void Server::sendError(int client_fd, int code, const Server_struct &server)
{
	Response res = Response::fromErrorCode(code, server);
	std::string msg = res.serializer();
	m_outbuf[client_fd] = msg;

	for (size_t j = 0; j < m_fds.size(); ++j)
	{
		if (m_fds[j].fd == client_fd)
		{
			m_fds[j].events = POLLIN | POLLOUT;
			break;
		}
	}
}

void Server::closeClientConnection(int client_fd, size_t &index)
{
	::close(client_fd);
	m_clientOrigin.erase(client_fd);
	m_inbuf.erase(client_fd);
	m_outbuf.erase(client_fd);
	m_fds.erase(m_fds.begin() + index);
	--index;
}

void Server::initializeListeners()
{
	for (size_t i = 0; i < m_config.servers.size(); ++i)
	{
		uint16_t port = static_cast<uint16_t>(m_config.servers[i].listen_port);
		
		if (m_seen.count(port))
			continue;

		m_seen.insert(port);
		int listenerFd = create_listener(port);
		m_listenerFdSet.insert(listenerFd);
		m_portToFd[port] = listenerFd;
		m_fdToPort[listenerFd] = port;
		m_fds.push_back({listenerFd, POLLIN, 0});

		std::cout << "Listening on http://localhost:" << port << "/\n";
	}
}

void Server::handlePollError(struct pollfd &pfd, size_t &index)
{
	if (!m_listenerFdSet.count(pfd.fd))
	{
		closeClientConnection(pfd.fd, index);
	}
	else
	{
		std::cout << "Ignoring POLLERR/POLLHUP for listener fd=" << pfd.fd << std::endl;
	}
}

void Server::acceptNewConnections(int listenerFd)
{
	while (true)
	{
		int newClientFd = ::accept(listenerFd, NULL, NULL);
		
		if (newClientFd < 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				break;
			perror("accept");
			break;
		}

		int cf = ::fcntl(newClientFd, F_GETFL, 0);
		if (cf < 0 || ::fcntl(newClientFd, F_SETFL, cf | O_NONBLOCK) < 0)
		{
			perror("fcntl(client)");
			::close(newClientFd);
			continue;
		}

		m_fds.push_back({newClientFd, POLLIN | POLLOUT, 0});
		m_clientOrigin[newClientFd] = listenerFd;
	}
}

void Server::handleClientWrite(struct pollfd &pfd, size_t &index)
{
	if (!m_outbuf.count(pfd.fd))
		return;

	std::string &outData = m_outbuf[pfd.fd];

	if (outData.empty())
		return;

	ssize_t sent = ::send(pfd.fd, outData.c_str(), outData.size(), 0);
	
	if (sent < 0)
	{
		perror("send");
		closeClientConnection(pfd.fd, index);
		return;
	}

	if (sent > 0)
	{
		outData.erase(0, sent);

		if (outData.empty())
		{
			m_outbuf.erase(pfd.fd);
			closeClientConnection(pfd.fd, index);
		}
	}
}

bool Server::readClientData(int client_fd)
{
	char buffer[4096];
	ssize_t recvRet = ::recv(client_fd, buffer, sizeof(buffer), 0);

	if (recvRet < 0)
	{
		perror("recv");
		return false;
	}

	if (recvRet == 0)
		return false;

	if (recvRet > 0)
		m_inbuf[client_fd].append(buffer, recvRet);

	return true;
}

void Server::setLocationDefaults(const Server_struct &server, 
								  const Location_struct *location,
								  std::string &docroot,
								  std::string &uploadDir,
								  std::string &indexName)
{
	if (location)
	{
		docroot = location->root.empty() ? server.root : location->root;
		uploadDir = location->upload_path.empty() ? "uploads" : location->upload_path;

		if (!location->index.empty())
			indexName = location->index[0];
		else if (!server.index.empty())
			indexName = server.index[0];
		else
			indexName = "index.html";
	}
	else
	{
		docroot = server.root;
		uploadDir = "uploads";
		indexName = server.index.empty() ? "index.html" : server.index[0];
	}
}

bool Server::checkCgiRequest(const Request &request, 
							  const Location_struct *location,
							  std::string &cgiExtension,
							  std::string &cgiInterpreterPath)
{
	if (!location || location->cgi_extension.empty())
		return false;

	cgiExtension = location->cgi_extension;
	cgiInterpreterPath = location->cgi_path;

	if (request.getPath().size() >= cgiExtension.size() &&
		request.getPath().rfind(cgiExtension) == request.getPath().size() - cgiExtension.size())
		return true;

	if (request.getPath() == location->path && !location->index.empty())
		return true;

	return false;
}

std::string Server::buildScriptPath(const Request &request,
									  const Location_struct *location,
									  const std::string &docroot,
									  const std::string &indexName)
{
	std::string targetPath = request.getPath();

	if (targetPath == location->path || targetPath.back() == '/')
	{
		if (targetPath.back() != '/')
			targetPath += "/";
		targetPath += indexName;
	}

	std::string scriptRelativePath = targetPath.substr(location->path.size());
	if (!scriptRelativePath.empty() && scriptRelativePath[0] == '/')
		scriptRelativePath.erase(0, 1);

	std::filesystem::path scriptPath;
	scriptPath = std::filesystem::weakly_canonical(std::filesystem::path(docroot) / scriptRelativePath);

	return scriptPath.string();
}

bool Server::validateScriptPath(const std::string &scriptPath, const std::string &docroot)
{
	if (::access(scriptPath.c_str(), F_OK | X_OK) != 0)
		return false;

	auto canonicalDocRoot = std::filesystem::weakly_canonical(docroot);
	if (scriptPath.rfind(canonicalDocRoot.string(), 0) != 0)
		return false;

	return true;
}

void Server::handleCgiRequest(int client_fd,
							   const Request &request,
							   const std::string &scriptPath,
							   const std::string &cgiInterpreterPath)
{
	CgiResult cg = runCgi(request, scriptPath, cgiInterpreterPath);
	cg.headers.clear();
	cg.headers["content-type"] = "text/html";

	Response res;
	int status = cg.status ? cg.status : 200;
	res.setStatus(status, reasonPhrase(status));

	for (const auto &kv : cg.headers)
		res.setHeader(kv.first, kv.second);

	res.setBody(cg.body);
	m_outbuf[client_fd] = res.serializer();

	for (size_t j = 0; j < m_fds.size(); ++j)
	{
		if (m_fds[j].fd == client_fd)
		{
			m_fds[j].events = POLLIN | POLLOUT;
			break;
		}
	}

	m_inbuf.erase(client_fd);
}

void Server::processCompleteRequest(int client_fd, size_t &index)
{
	RequestParser parser;
	Request request;

	int listener_fd = m_clientOrigin[client_fd];
	uint16_t port = m_fdToPort[listener_fd];
	const Server_struct *current_server = findServerByPort(m_config, port);

	if (!current_server)
	{
		std::cerr << "ERROR: Could not find server for port " << port << std::endl;
		closeClientConnection(client_fd, index);
		return;
	}

	try
	{
		request = parser.parse(m_inbuf[client_fd]);

		std::string contentLength = request.getHeader("Content-Length");
		if (!contentLength.empty())
		{
			size_t bodySize = std::strtoul(contentLength.c_str(), NULL, 10);
			if (bodySize > current_server->client_max_body_size)
			{
				sendError(client_fd, 413, *current_server);
				closeClientConnection(client_fd, index);
				return;
			}
		}
	}
	catch (const std::exception &e)
	{
		std::cerr << "Parse error: " << e.what() << std::endl;
		sendError(client_fd, 400, *current_server);
		closeClientConnection(client_fd, index);
		return;
	}

	std::string requested_path = request.getPath();
	const Location_struct *matchedLocation = matchLocation(*current_server, requested_path);

	std::string docroot, uploadDir, indexName;
	setLocationDefaults(*current_server, matchedLocation, docroot, uploadDir, indexName);

	std::string cgiExtension, cgiInterpreterPath;
	bool isCgi = checkCgiRequest(request, matchedLocation, cgiExtension, cgiInterpreterPath);

	if (isCgi)
	{
		std::string scriptPath = buildScriptPath(request, matchedLocation, docroot, indexName);

		if (!validateScriptPath(scriptPath, docroot))
		{
			int errorCode = (::access(scriptPath.c_str(), F_OK) != 0) ? 404 : 403;
			sendError(client_fd, errorCode, *current_server);
			closeClientConnection(client_fd, index);
			return;
		}

		handleCgiRequest(client_fd, request, scriptPath, cgiInterpreterPath);
		return;
	}

	Router router(docroot, uploadDir, indexName, *current_server);
	Response res = router.handleRequest(request);
	m_outbuf[client_fd] = res.serializer();

	for (size_t j = 0; j < m_fds.size(); ++j)
	{
		if (m_fds[j].fd == client_fd)
		{
			m_fds[j].events = POLLIN | POLLOUT;
			break;
		}
	}
}

void Server::handleClientRead(struct pollfd &pfd, size_t &index)
{
	if (!readClientData(pfd.fd))
	{
		closeClientConnection(pfd.fd, index);
		return;
	}

	std::string &rawRequest = m_inbuf[pfd.fd];
	if (!HTTP_IsRequestComplete(rawRequest))
		return;

	processCompleteRequest(pfd.fd, index);
}

int Server::start_server(void)
{
	if (m_config.servers.empty())
	{
		std::cerr << "Error: No servers loaded from config.\n";
		return 1;
	}

	initializeListeners();

	while (true)
	{
		int ret = ::poll(m_fds.data(), m_fds.size(), -1);
		if (ret < 0)
		{
			perror("poll");
			std::exit(1);
		}

		for (size_t i = 0; i < m_fds.size(); ++i)
		{
			struct pollfd &pfd = m_fds[i];

			if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL))
			{
				handlePollError(pfd, i);
				continue;
			}

			if (m_listenerFdSet.count(pfd.fd))
			{
				acceptNewConnections(pfd.fd);
				continue;
			}

			if ((pfd.revents & POLLOUT) && m_outbuf.count(pfd.fd))
			{
				handleClientWrite(pfd, i);
				continue;
			}

			if (pfd.revents & POLLIN)
			{
				handleClientRead(pfd, i);
			}
		}
	}

	return 0;
}