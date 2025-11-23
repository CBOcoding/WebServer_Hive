#pragma once

#include"headers.hpp"

class	Config
{
private:
	int			m_port;
	std::string	m_root;

protected:

public:
	Config();
	~Config();
	Config(const Config& other) = delete;
	Config& operator=(const Config& other) = delete;

	int			getPort() const;
	std::string	getRoot() const;


};