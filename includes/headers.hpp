#pragma once

#define MB 1024 * 1024

#include <iostream>
#include <sys/socket.h> // socket()
#include <sys/types.h>
#include <netdb.h>	// sockaddr_in (struct)
#include <unistd.h> // close() and access()
#include <fcntl.h>	// fcntl()
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <poll.h>
#include <map>
#include <cstring>
#include <algorithm>  // parser
#include <sys/stat.h> // for stat() function
#include <set>
#include <utility>
#include <stdexcept>
#include <filesystem>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sys/wait.h>
#include <sys/select.h>
#include <signal.h>
#include <errno.h>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <sstream>
#include <fstream>
#include <iostream>
#include <cstring>
#include <cctype>
#include <netinet/in.h>





#include "ConfigStructs.hpp"
#include "Server.hpp"
#include "Response.hpp"
#include "Request.hpp"
#include "RequestParser.hpp"
#include "utils.hpp"
#include "Router.hpp"
#include "Config.hpp"
#include "ConfigParser.hpp"
#include "ConfigCheck.hpp"
#include "Cgi.hpp"
#include "http_utils.hpp"
