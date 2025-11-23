#pragma once

#include "headers.hpp"


class ConfigParser {
private:
    std::string filename;
    Config_struct config; // will hold parsed servers

    void parseLine(const std::string& line, Server_struct& server, Location_struct& location, bool& inLocation, int lineNumber);

protected:

public:
	ConfigParser();
	~ConfigParser() = default;
	ConfigParser (const ConfigParser& other) = delete;
	ConfigParser& operator=(const ConfigParser& other) = delete;
	
	ConfigParser(const std::string& file);

	// parse the config file
	bool parse();

	// get the final Config
	const Config_struct& getConfig() const;
};
