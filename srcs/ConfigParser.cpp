// #include "ConfigParser.hpp"
#include "headers.hpp"

ConfigParser::ConfigParser(const std::string& file) : filename(file) {}

const	Config_struct& ConfigParser::getConfig() const {
	return config;
}

bool ConfigParser::parse() {
    std::ifstream file(filename.c_str());
    if (!file.is_open()) {
        std::cerr << "Failed to open config file: " << filename << std::endl;
        return false;
    }

    Server_struct currentServer;
    Location_struct currentLocation;
    bool inServer = false;
    bool inLocation = false;
    std::string line;
	int	braceCount = 0;
	int	lineNumber = 0; // track line number

    while (std::getline(file, line)) {
		lineNumber++;
        // remove leading/tailing spaces (everything on the left first and on the right after)
        line.erase(0, line.find_first_not_of(" \t\n\r")); // space, tab, new line, carriage return.
        line.erase(line.find_last_not_of(" \t\n\r") + 1); // find the last character that is not whitespace.
		// the +1 is needed because erase starts to erase from the pos position to the end of the string.

		// Remove inline comments (everything after #)
		size_t commentPos = line.find('#');
		if (commentPos != std::string::npos) {
			line = line.substr(0, commentPos);
			// Trim trailing whitespace after removing comment
			line.erase(line.find_last_not_of(" \t\n\r") + 1);
		}

		if (line.empty() || line[0] == '#')
			continue; // skip empty lines and comments

		if (line.size() >= 7 && line.substr(0, 6) == "server" && line.back() == '{') {
			int	end = (line.size() - 1);

			for (int i = 6; i < end; i++) {
				if (!isspace(line[i]))
					throw std::runtime_error("Configuration error: server line.");
			}

			braceCount++;
			inServer = true;
			currentServer = Server_struct(); // reset
			currentServer.listen_port = 8080;
			currentServer.root = "www";
			currentServer.client_max_body_size = 1048576;

			// Default error pages
			for (int code : {404, 403, 500}) {
				if (currentServer.error_pages.find(code) == currentServer.error_pages.end())
					currentServer.error_pages[code] = "/error.html";
			}
// //debug
// std::cout << "Server added! listen_port=" << currentServer.listen_port << std::endl;
// //end debug
				continue;
		}

		//Block closing
		if (line == "}") {
			if (inLocation) {
				if (currentLocation.root.empty())
					currentLocation.root = currentServer.root;

				if (currentLocation.methods.empty()) {
					// std::cerr << "Warning: location " << currentLocation.path << " has no allowed HTTP methods. Defaulting to GET.\n";
					currentLocation.methods.push_back("GET");
				}

				currentServer.locations.push_back(currentLocation);
				inLocation = false;
			}
			else if (inServer) {

				if (currentServer.index.empty())
					currentServer.index.push_back("index.html");

				config.servers.push_back(currentServer);
				inServer = false;
// //debug
// std::cout << "CONFIGPARSER Server loaded: listen=" << currentServer.listen_port << ", root=" << currentServer.root << std::endl;
// //end debug
			}
			braceCount--;
			continue;
		}

		if (line.find('{') != std::string::npos)
		braceCount++;

		if (braceCount < 0)
			throw std::runtime_error("Unexpected closing brace '}' in config");

		// Parse the actual directive line
		parseLine(line, currentServer, currentLocation, inLocation, lineNumber);
	}

	if (braceCount != 0)
		throw std::runtime_error("Unmatched braces in config file");

	// Validate unique (host:port, server_name) combinations
	std::set<std::pair<int, std::string>> seen;
	for (size_t i = 0; i < config.servers.size(); ++i) {
		const Server_struct& s = config.servers[i];

		// if no server_name given, treat as "default_server"
		if (s.server_names.empty()) {
			std::pair<int, std::string> key(s.listen_port, "default_server");
			if (!seen.insert(key).second) {
				throw std::runtime_error("Duplicate (port, server_name) binding: port " + std::to_string(s.listen_port) + " server_name=default_server");
			}
		}
		else {
			for (std::vector<std::string>::const_iterator it = s.server_names.begin(); it != s.server_names.end(); ++it) {
				std::pair<int, std::string> key(s.listen_port, *it);
				if (!seen.insert(key).second) {
					throw std::runtime_error("Duplicate (port, server_name) binding: port " + std::to_string(s.listen_port) + " server_name=" + *it);
				}
			}
		}
	}

	//Require at least one location per server
	for (size_t i = 0; i < config.servers.size(); ++i) {
		const Server_struct &s = config.servers[i];
		if (s.locations.empty()) {
			throw std::runtime_error("Configuration error: server on port " + std::to_string(s.listen_port) + " has no location blocks defined.");
		}
	}


	file.close();
	return true;
}

void	ConfigParser::parseLine(const std::string& line, Server_struct& server, Location_struct& location, bool& inLocation, int lineNumber) {
	std::istringstream iss(line); // iss works as a small input file containing all the words of line.
	std::string directive;
	iss >> directive; // This pulls the first word from the line.

	// Helper function to remove trailing semicolon
	auto removeSemicolon = [](int lineNumber, std::string& str) {
		if (str.back() != ';') {
			throw std::runtime_error("Missing semicolon at the end of line: " + std::to_string(lineNumber));
		}
		if (!str.empty() && str.back() == ';') {
			str.pop_back();
		}
	};

	if (directive == "listen") {
		std::string portStr;
		std::string extra;
		if (!(iss >> portStr))
			throw std::runtime_error("File " + filename + " Line " + std::to_string(lineNumber) + ": Missing value in port");
		removeSemicolon(lineNumber, portStr);
		if (iss >> extra)
			throw std::runtime_error("File " + filename + " Line " + std::to_string(lineNumber) + ": Too many Args in port");

		int port;
		try {
			port = std::stoi(portStr);
		}
		catch (const std::invalid_argument& e) {
			throw std::runtime_error("File " + filename + " Line " + std::to_string(lineNumber) + ": Invalid port number: " + portStr);
		}
		catch (const std::out_of_range& e) {
			throw std::runtime_error("File " + filename + " Line " + std::to_string(lineNumber) + ": Port number out of range: " + portStr);
		}

		if (port <= 0 || port > 65535)
			throw std::runtime_error("File " + filename + " Line " + std::to_string(lineNumber) + ": Invalid port number: " + std::to_string(port));
		if (port < 1024)
			std::cerr << "Warning, in Linux ports < 1024 require root permissions." << std::endl;

		server.listen_port = port;
	}

	else if (directive == "server_name") {
		std::string name;
		while (iss >> name) {
			removeSemicolon(lineNumber, name);
		server.server_names.push_back(name);
		}
	}

	else if (directive == "root") {
		std::string path;
		std::string extra;
		if (!(iss >> path))
			throw std::runtime_error("File " + filename + " Line " + std::to_string(lineNumber) + ": Missing value in root");
		removeSemicolon(lineNumber, path);
		if (iss >> extra)
			throw std::runtime_error("File " + filename + " Line " + std::to_string(lineNumber) + ": Too many Args in root");

		// Check if path exists using stat
		struct stat statbuf;
		if (stat(path.c_str(), &statbuf) != 0)
			throw std::runtime_error("File " + filename + " Line " + std::to_string(lineNumber) + ": Root path does not exist: " + path);

		if (inLocation)
			location.root = path;
		else
			server.root = path;
	}

	else if (directive == "index") {
		std::string idx;
		while (iss >> idx) {
			removeSemicolon(lineNumber, idx);
			if (inLocation)
				location.index.push_back(idx);
			else
				server.index.push_back(idx);
		}
	}


	else if (directive == "location") {
		location = Location_struct(); // reset location
		std::string pathAndBrace;
		if (!(iss >> pathAndBrace))
			throw std::runtime_error("File " + filename + " Line " + std::to_string(lineNumber) + ": Missing value in location");

		// Handle "location / {" format - remove the trailing {
		if (pathAndBrace[0] != '/')
			throw std::runtime_error("File " + filename + " Line " + std::to_string(lineNumber) + ": Missing '/'");
		size_t secondSlash = pathAndBrace.find('/', 1);
		if (secondSlash != std::string::npos)
			throw std::runtime_error("File " + filename + " Line " + std::to_string(lineNumber) + ": Extra '/'");

		size_t oppositeSlash = pathAndBrace.find(92, 1);
		if (oppositeSlash != std::string::npos)
			throw std::runtime_error("File " + filename + " Line " + std::to_string(lineNumber) + ": Extra '\\'");


		if (pathAndBrace.back() == '{') {
			location.path = pathAndBrace.substr(0, pathAndBrace.length() - 1);
		}
		else {
			location.path = pathAndBrace;
		}

		// Check if there are extra arguments after handling the path
		std::string extra;
		if (iss >> extra && extra != "{")
			throw std::runtime_error("File " + filename + " Line " + std::to_string(lineNumber) + ": Too many Args in location");

		inLocation = true;
	}

	else if (directive == "methods") {
		std::string method;

		if (!inLocation) {
			throw std::runtime_error("File " + filename + " Line " + std::to_string(lineNumber) + ": Error: 'methods' directive only allowed inside location blocks\n");
		}

		bool	semicolonFound = false;

		while (iss >> method) {
			// //remove trailing semicolon
			if (!method.empty() && method.back() == ';') {
				semicolonFound = true;
				method.pop_back();

				if (!method.empty())
					location.methods.push_back(method);

				break;
			}
			location.methods.push_back(method);
		}
		if (!semicolonFound) {
			throw std::runtime_error("Missing semicolon at the end of line: " + std::to_string(lineNumber));
		}
	}

	/**
	 * 100–199 → informational responses (e.g. 100 Continue)
	 * 200–299 → success (e.g. 200 OK, 201 Created)
	 * 300–399 → redirects (e.g. 301 Moved Permanently, 302 Found)
	 * 400–499 → client errors (e.g. 404 Not Found, 403 Forbidden)
	 * 500–599 → server errors (e.g. 500 Internal Server Error, 502 Bad Gateway)
	 */
	// We can add other directives similarly (error_page, client_max_body_size, upload_path, etc.)
	else if (directive == "error_page") {
		int error_code;
		std::string error_path;
		if (!(iss >> error_code >> error_path))
			throw std::runtime_error("File " + filename + " Line " + std::to_string(lineNumber) + ": Invalid error_page directive: " + line);
		if (error_code < 100 || error_code > 599)
			throw std::runtime_error("File " + filename + " Line " + std::to_string(lineNumber) + ": Invalid HTTP error code: " + std::to_string(error_code));

        while (!error_path.empty() && (error_path.back() == ';' || std::isspace(error_path.back())))
            error_path.pop_back();
		server.error_pages[error_code] = error_path;
	}

	else if (directive == "client_max_body_size") {
		std::string sizeStr;
		std::string extra;
		if (!(iss >> sizeStr))
			throw std::runtime_error("File " + filename + " Line " + std::to_string(lineNumber) + ": Missing value in client_max_body_size");
		removeSemicolon(lineNumber, sizeStr);
		if (iss >> extra)
			throw std::runtime_error("File " + filename + " Line " + std::to_string(lineNumber) + ": Too many Args in client_max_body_size");

		long size;
		try {
			size = std::stol(sizeStr);
		}
		catch (const std::invalid_argument& e) {
			throw std::runtime_error("File " + filename + " Line " + std::to_string(lineNumber) + ": Invalid client_max_body_size number: " + sizeStr);
		}
		catch (const std::out_of_range& e) {
			throw std::runtime_error("File " + filename + " Line " + std::to_string(lineNumber) + ": client_max_body_size number out of range: " + sizeStr);
		}

		if (size <= 0 || size > 100 * MB) 
			throw std::runtime_error("File " + filename + " Line " + std::to_string(size) + " is an invalid client_max_body_size");

		server.client_max_body_size = size;
	}

	else if (directive == "upload_path") {
		std::string path;
		std::string extra;
		if (!(iss >> path))
			throw std::runtime_error("File " + filename + " Line " + std::to_string(lineNumber) + ": Missing path in upload_path");
		removeSemicolon(lineNumber, path);
		if (iss >> extra)
			throw std::runtime_error("File " + filename + " Line " + std::to_string(lineNumber) + ": Too many Args in upload_path");

		// Check if path exists using stat
		struct stat statbuf;
		if (stat(path.c_str(), &statbuf) != 0)
			throw std::runtime_error("File " + filename + " Line " + std::to_string(lineNumber) + ": Upload path does not exist: " + path);

		location.upload_path = path;
	}

	else if (directive == "redirect") {
		std::string redirection;
		std::string extra;
		if (!(iss >> redirection))
			throw std::runtime_error("File " + filename + " Line " + std::to_string(lineNumber) + ": Missing path in redirect");
		removeSemicolon(lineNumber, redirection);
		if (iss >> extra)
			throw std::runtime_error("File " + filename + " Line " + std::to_string(lineNumber) + ": Too many Args in redirect");
		location.redirect = redirection;
	}

	else if (directive == "cgi_extension") {
		std::string extension;
		std::string extra;
		if (!(iss >> extension))
			throw std::runtime_error("File " + filename + " Line " + std::to_string(lineNumber) + ": Missing extension in cgi_extension");
		removeSemicolon(lineNumber, extension);
		if (iss >> extra)
			throw std::runtime_error("File " + filename + " Line " + std::to_string(lineNumber) + ": Too many Args in cgi_extension");

		if (extension[0] != '.')
			throw std::runtime_error("File " + filename + " Line " + std::to_string(lineNumber) + ": cgi_extension must start with a dot: " + extension);

		size_t i = 1; //skipping 0 because there is a dot in 0 (previous check).
		while (i < extension.size()) {
			char c = extension[i];
			if (!isalnum(c) && c != '_' && c != '-') {
				throw std::runtime_error("File " + filename + " Line " + std::to_string(lineNumber) + ": cgi_extension contains invalid character: " + extension);
			}
			i++;
		}
		location.cgi_extension = extension;
	}

	else if (directive == "cgi_path") {
		std::string path;
		std::string extra;
		if (!(iss >> path))
			throw std::runtime_error("File " + filename + " Line " + std::to_string(lineNumber) + ": Missing value in cgi_path");
		removeSemicolon(lineNumber, path);
		if (iss >> extra)
			throw std::runtime_error("File " + filename + " Line " + std::to_string(lineNumber) + ": Too many Args in cgi_path");
		location.cgi_path = path;
	}
}


