#include "headers.hpp"

Config::Config() {
	m_port = 8082;
	m_root = "www/";
}

Config::~Config() {}

int	Config::getPort() const {
	return m_port;
}

std::string	Config::getRoot() const {
	return m_root;
}