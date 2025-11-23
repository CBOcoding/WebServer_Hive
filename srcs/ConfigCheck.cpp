#include "ConfigCheck.hpp"

void	configFinalCheck(const Config_struct &config) {
	if (config.servers.empty())
        throw std::runtime_error("No servers defined in config!");

    for (size_t i = 0; i < config.servers.size(); ++i) {
        const Server_struct &srv = config.servers[i];

        // Server-level checks
        if (srv.listen_port <= 0 || srv.listen_port > 65535)
            throw std::runtime_error("Server " + std::to_string(i) + " has invalid listen port!");

        if (srv.root.empty())
            throw std::runtime_error("Server " + std::to_string(i) + " has empty root path!");

        if (srv.index.empty())
            throw std::runtime_error("Server " + std::to_string(i) + " has no index files defined!");

        if (srv.server_names.empty())
            throw std::runtime_error("Server " + std::to_string(i) + " has no server_name defined!");

        if (srv.client_max_body_size <= 0 || srv.client_max_body_size > 100*MB)
            throw std::runtime_error("Server " + std::to_string(i) + " has invalid client_max_body_size!");

        for (int code : {400, 403, 404, 500}) {
            if (srv.error_pages.find(code) == srv.error_pages.end()) {
                std::cerr << "Warning: Server " << i << " missing error_page for code " << code << std::endl;
            }
        }
		for (std::map<int, std::string>::const_iterator it = srv.error_pages.begin(); it != srv.error_pages.end(); ++it) {
			int code = it->first;
			if (code < 100 || code > 599) {
				throw std::runtime_error("Server " + std::to_string(i) + " has invalid error_page code: " + std::to_string(code));
			}
		}


        // Location-level checks
        for (size_t j = 0; j < srv.locations.size(); ++j) {
            const Location_struct &loc = srv.locations[j];

            if (loc.path.empty())
                throw std::runtime_error("Server " + std::to_string(i) + " location " + std::to_string(j) + " has empty path!");

            if (loc.methods.empty())
                throw std::runtime_error("Server " + std::to_string(i) + " location " + std::to_string(j) + " has no allowed methods!");

            if (loc.root.empty())
                throw std::runtime_error("Server " + std::to_string(i) + " location " + std::to_string(j) + " has empty root!");

            if (!loc.upload_path.empty()) {
                struct stat statbuf;
                if (stat(loc.upload_path.c_str(), &statbuf) != 0)
                    throw std::runtime_error("Server " + std::to_string(i) + " location " + std::to_string(j) + " upload_path does not exist: " + loc.upload_path);
            }

            if (!loc.redirect.empty() && loc.redirect[0] != '/')
                throw std::runtime_error("Server " + std::to_string(i) + " location " + std::to_string(j) + " redirect must start with '/'!");

            if (!loc.cgi_extension.empty()) {
                if (loc.cgi_extension[0] != '.')
                    throw std::runtime_error("Server " + std::to_string(i) + " location " + std::to_string(j) + " cgi_extension must start with '.'");

                for (size_t k = 1; k < loc.cgi_extension.size(); ++k) {
                    char c = loc.cgi_extension[k];
                    if (!isalnum(c) && c != '_' && c != '-')
                        throw std::runtime_error("Server " + std::to_string(i) + " location " + std::to_string(j) + " cgi_extension contains invalid character");
				}
				
				if (loc.cgi_path.empty())
					throw std::runtime_error("Server " + std::to_string(i) + " location " + std::to_string(j) + " has cgi_extension but no cgi_path defined!");
				else {
					struct stat statbuf;
					if (stat(loc.cgi_path.c_str(), &statbuf) != 0)
						throw std::runtime_error("Server " + std::to_string(i) + " location " + std::to_string(j) + " CGI path does not exist: " + loc.cgi_path);
				}
			}
		}

	//check HTTP methods
	const std::vector<std::string> validMethods = {"GET", "POST", "PUT", "DELETE", "PATCH", "HEAD", "OPTIONS"};

    for (size_t si = 0; si < config.servers.size(); ++si) {
        const Server_struct &server = config.servers[si];

        for (size_t li = 0; li < server.locations.size(); ++li) {
            const Location_struct &loc = server.locations[li];
            for (const auto &method : loc.methods) {
                // Check the method is fully uppercase
				for (char c : method) {
					if (!isupper(c))
						throw std::runtime_error("HTTP method must be uppercase: " + method);
				}

				// Check it is an allowed methods
				if (std::find(validMethods.begin(), validMethods.end(), method) == validMethods.end()) {
					throw std::runtime_error("Server " + std::to_string(si) + " location " + std::to_string(li) + " has invalid HTTP method: " + method);
				}

            }
        }
    }

	//check index file existence
	for (size_t si = 0; si < config.servers.size(); ++si) {
		const Server_struct &server = config.servers[si];
		struct stat statbuf;
		
		for (const auto &idx : server.index) {
	std::string fullPath = server.root + "/" + idx;
				if (stat(fullPath.c_str(), &statbuf) != 0) {
					std::cerr << "Warning: Server " << si << " index file does not exist: " << fullPath << std::endl;
				}
		}

		for (size_t li = 0; li < server.locations.size(); ++li) {
			const Location_struct &loc = server.locations[li];
			for (const auto &idx : loc.index) {
				std::string fullPath = loc.root + "/" + idx;
				if (stat(fullPath.c_str(), &statbuf) != 0) {
					std::cerr << "Warning: Server " << si << " location " << li << " index file does not exist: " << fullPath << std::endl;
				}
			}
		}

		//check root and upload_path
		if (stat(server.root.c_str(), &statbuf) != 0)
			throw std::runtime_error("Server " + std::to_string(si) + " root path does not exist: " + server.root);

		for (size_t li = 0; li < server.locations.size(); ++li) {
			const Location_struct &loc = server.locations[li];

			if (!loc.root.empty() && stat(loc.root.c_str(), &statbuf) != 0)
					throw std::runtime_error("Server " + std::to_string(si) + " location " + std::to_string(li) + " root path does not exist: " + loc.root);

			if (!loc.upload_path.empty() && stat(loc.upload_path.c_str(), &statbuf) != 0)
					throw std::runtime_error("Server " + std::to_string(si) + " location " + std::to_string(li) + " upload path does not exist: " + loc.upload_path);

			}
		}
	}
}