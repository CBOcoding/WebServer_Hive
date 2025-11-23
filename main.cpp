#include "headers.hpp"

int main(int argc, char **argv)
{

	try
	{
		std::string configFile;

		if (argc != 1 && argc != 2)
			throw std::runtime_error("Wrong number of Args.");

		if (argc == 1)
			configFile = "full_test.conf"; // setting the default conf
		else if (argc == 2)
			configFile = argv[1];

		// Check if config file exists using stat
		struct stat statbuf;
		if (stat(configFile.c_str(), &statbuf) != 0)
		{
			throw std::runtime_error("Config file not found: " + configFile);
		}

		ConfigParser parser(configFile);
		if (!parser.parse())
		{
			throw std::runtime_error("Failed to parse config file: " + configFile);
		}

		Config_struct config = parser.getConfig();

		// final check of the config struct
		configFinalCheck(config);

		Server server(config);

		// Server server;
		server.start_server();
	}
	catch (const std::exception &e)
	{
		std::cerr << "Configuration error: " << e.what() << std::endl;
		return 1;
	}
	return 0;
}
