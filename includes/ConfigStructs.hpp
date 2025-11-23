#pragma once

/**
 * path → tells the server which URLs this location applies to
 * root → where files are read from
 * index → default file if URL points to a folder
 * methods → HTTP methods allowed for this location
 * upload_path →  folder to store uploaded files
 * cgi_extension → if requests with this extension should trigger CGI
 * redirect →  HTTP redirect
 */

struct Location_struct {
	std::string					path;
	std::string					root;
	std::vector<std::string>	index;
	std::vector<std::string>	methods;
	std::string					upload_path;
	std::string					cgi_extension;
	std::string					cgi_path;
	std::string					redirect;
	int							redirect_code; // 301, 302, 307, 308
    std::string					redirect_url; // target URL
};

/**
 * listen_port → the port number for this virtual server
 * server_names → names like localhost or www.example.com
 * root → default root folder for the server
 * index → default index file if URL is /
 * client_max_body_size → limit for POST/PUT requests
 * error_pages → maps HTTP codes (404, 500) to files
 * locations → list of all Location structs for sub-paths
 */

struct Server_struct {
	int								listen_port;
	std::vector<std::string>		server_names;
	std::string						root;
	std::vector<std::string>		index;
	size_t							client_max_body_size;
	std::map<int, std::string>		error_pages;
	std::vector<Location_struct>	locations;
};

/**
 * servers → a vector of Server objects
 * Each Server can have multiple Location blocks
 */
struct Config_struct {
	std::vector<Server_struct>			servers;
};
