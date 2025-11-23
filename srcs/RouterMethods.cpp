#include "headers.hpp"

static std::string extractNameFromPath(const std::string &path)
{
	if (path.empty())
		return "";

	size_t pos = path.find_last_of('/');
	std::string name = (pos == std::string::npos) ? path : path.substr(pos + 1);

	while (!name.empty() && (name.front() == '/' || name.front() == '\\'))
		name.erase(0, 1);

	if (name.find('/') != std::string::npos || name.find('\\') != std::string::npos)
		return "";
	if (name == "." || name == "..")
		return "";
	return name;
}

static std::string makeTimestampName(const std::string &ext = ".bin")
{
	using namespace std::chrono;
	auto now = system_clock::now();
	std::time_t t = system_clock::to_time_t(now);
	std::tm tm{};
#ifdef _WIN32
	localtime_s(&tm, &t);
#else
	localtime_r(&t, &tm);
#endif
	char buf[32];
	std::strftime(buf, sizeof(buf), "upload_%Y%m%d_%H%M%S", &tm);
	return std::string(buf) + ext;
}

static std::string decodeChunkedBody(const std::string &raw)
{
    std::string decoded;
    std::istringstream ss(raw);
    std::string line;
    while (std::getline(ss, line))
    {
        if (line.empty()) continue;
        size_t chunkSize = std::stoul(line, nullptr, 16);
        if (chunkSize == 0) break;
        std::string chunk(chunkSize, '\0');
        ss.read(&chunk[0], chunkSize);
        decoded += chunk;
        ss.ignore(2); // \r\n
    }
    return decoded;
}

Response Router::handleGET(const std::string &path)
{
    Response res;
    std::string fullPath = m_docroot + path;

    std::filesystem::path canonicalFull = std::filesystem::weakly_canonical(fullPath);
    std::filesystem::path canonicalRoot = std::filesystem::weakly_canonical(m_docroot);

    if (canonicalFull.string().find(canonicalRoot.string()) != 0)
        return Response::fromErrorCode(403, m_serverConfig);

    if (!std::filesystem::exists(canonicalFull))
    {
        std::filesystem::path alt = canonicalFull;
        alt += ".html";
        if (std::filesystem::exists(alt))
            canonicalFull = alt;
        else
            return Response::fromErrorCode(404, m_serverConfig);
    }

 
    if (std::filesystem::is_directory(canonicalFull))
    {
        std::filesystem::path indexPath = canonicalFull / (m_index.empty() ? "index.html" : m_index);
        
        if (std::filesystem::exists(indexPath))
        {
      
            canonicalFull = indexPath;
        }
        else
        {

            return generateDirectoryListing(canonicalFull.string(), path);
        }
    }

    res.setFilePath(canonicalFull.string());
    res.loadFile();

    res.setHeader("Content-Type", getContentType(canonicalFull.string()));
    res.setHeader("Content-Length", std::to_string(res.getBody().size()));

    return res;
}


Response Router::generateDirectoryListing(const std::string &fullPath, const std::string &urlPath)
{
    Response res;
    std::ostringstream html;

 
    html << "<!DOCTYPE html>\n"
         << "<html>\n<head>\n"
         << "<meta charset=\"UTF-8\">\n"
         << "<title>Index of " << urlPath << "</title>\n"
         << "<style>\n"
         << "body { font-family: Arial, sans-serif; margin: 40px; }\n"
         << "h1 { border-bottom: 2px solid #333; padding-bottom: 10px; }\n"
         << "table { border-collapse: collapse; width: 100%; }\n"
         << "th { text-align: left; padding: 12px; background-color: #f0f0f0; border-bottom: 2px solid #ddd; }\n"
         << "td { padding: 10px; border-bottom: 1px solid #eee; }\n"
         << "tr:hover { background-color: #f5f5f5; }\n"
         << "a { color: #0066cc; text-decoration: none; }\n"
         << "a:hover { text-decoration: underline; }\n"
         << ".dir { font-weight: bold; }\n"
         << ".size { text-align: right; }\n"
         << "</style>\n"
         << "</head>\n<body>\n"
         << "<h1>Index of " << urlPath << "</h1>\n"
         << "<table>\n"
         << "<tr><th>Name</th><th>Size</th><th>Type</th></tr>\n";

    if (urlPath != "/" && !urlPath.empty())
    {
        std::string parentPath = urlPath;
        if (parentPath.back() == '/')
            parentPath.pop_back();
        size_t lastSlash = parentPath.find_last_of('/');
        parentPath = (lastSlash == std::string::npos) ? "/" : parentPath.substr(0, lastSlash + 1);
        
        html << "<tr><td><a href=\"" << parentPath << "\" class=\"dir\">[Parent Directory]</a></td>"
             << "<td class=\"size\">-</td><td>Directory</td></tr>\n";
    }


    try
    {
        std::vector<std::filesystem::directory_entry> entries;
        for (const auto &entry : std::filesystem::directory_iterator(fullPath))
            entries.push_back(entry);


        std::sort(entries.begin(), entries.end(), 
            [](const auto &a, const auto &b) {
                bool aIsDir = a.is_directory();
                bool bIsDir = b.is_directory();
                if (aIsDir != bIsDir)
                    return aIsDir;
                return a.path().filename().string() < b.path().filename().string();
            });

        for (const auto &entry : entries)
        {
            std::string name = entry.path().filename().string();
            std::string linkPath = urlPath;
            if (linkPath.back() != '/')
                linkPath += '/';
            linkPath += name;

            html << "<tr>";
            
            if (entry.is_directory())
            {
                html << "<td><a href=\"" << linkPath << "/\" class=\"dir\">" << name << "/</a></td>"
                     << "<td class=\"size\">-</td>"
                     << "<td>Directory</td>";
            }
            else
            {
                std::uintmax_t size = std::filesystem::file_size(entry.path());
                std::string sizeStr = formatFileSize(size);
                
                html << "<td><a href=\"" << linkPath << "\">" << name << "</a></td>"
                     << "<td class=\"size\">" << sizeStr << "</td>"
                     << "<td>File</td>";
            }
            
            html << "</tr>\n";
        }
    }
    catch (const std::filesystem::filesystem_error &e)
    {
        return Response::fromErrorCode(500, m_serverConfig);
    }

    html << "</table>\n</body>\n</html>";

    std::string body = html.str();
    res.setBody(body);
    res.setStatus(200, "OK");
    res.setHeader("Content-Type", "text/html; charset=UTF-8");
    res.setHeader("Content-Length", std::to_string(body.size()));

    return res;
}


std::string Router::formatFileSize(std::uintmax_t bytes)
{
    const char* units[] = {"B", "KB", "MB", "GB"};
    int unitIndex = 0;
    double size = static_cast<double>(bytes);

    while (size >= 1024.0 && unitIndex < 3)
    {
        size /= 1024.0;
        unitIndex++;
    }

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << size << " " << units[unitIndex];
    return oss.str();
}

Response Router::handlePOST(const Request &req)
{
    Response res;

    std::string contentLength = req.getHeader("Content-Length");
    if (contentLength.empty() && req.getHeader("Transfer-Encoding") != "chunked")
        return Response::fromErrorCode(411, m_serverConfig);

    if (!contentLength.empty() && std::stoi(contentLength) > 100 * MB)
        return Response::fromErrorCode(413, m_serverConfig);

    std::string name = extractNameFromPath(req.getPath());
    if (name.empty())
    {
        std::string cd = req.getHeader("Content-Disposition");
        name = extractFilename(cd);
    }
    if (name.empty())
        name = makeTimestampName(".bin");

    std::filesystem::path base = std::filesystem::weakly_canonical(m_docroot) / m_uploads;
    std::filesystem::create_directories(base);

    std::filesystem::path outp = base / name;
    std::filesystem::path canonicalOut = std::filesystem::weakly_canonical(outp);
    if (canonicalOut.string().find(base.string()) != 0)
        return Response::fromErrorCode(403, m_serverConfig);

    if (std::filesystem::exists(canonicalOut))
        return Response::fromErrorCode(409, m_serverConfig);

    std::string body;
    if (req.getHeader("Transfer-Encoding") == "chunked")
        body = decodeChunkedBody(req.getRawBody());
    else
        body = req.getBody();

    std::ofstream out(canonicalOut, std::ios::binary);
    if (!out.is_open())
        return Response::fromErrorCode(500, m_serverConfig);
    out.write(body.data(), body.size());
    out.close();

    std::string okBody = "Created\n";
    res.setStatus(201, "Created");
    res.setHeader("Content-Type", "text/plain");
    res.setHeader("Content-Length", std::to_string(okBody.size()));
    res.setBody(okBody);

    return res;
}

Response Router::handleDELETE(const std::string &path)
{
    std::string filename;
    if (path.find("/uploads/") == 0)
        filename = path.substr(9);
    else
        return Response::fromErrorCode(404, m_serverConfig);

    std::filesystem::path base = std::filesystem::path(m_docroot) / m_uploads;
    std::filesystem::path outp = base / filename;

    std::filesystem::path canonicalFull = std::filesystem::weakly_canonical(outp);
    std::filesystem::path canonicalUploads = std::filesystem::weakly_canonical(base);

    if (canonicalFull.string().find(canonicalUploads.string()) != 0)
    {
        std::cerr << "Access denied: " << canonicalFull.string() << " is outside " << canonicalUploads.string() << std::endl;
        return Response::fromErrorCode(403, m_serverConfig);
    }

    if (!std::filesystem::exists(canonicalFull))
    {
        std::cerr << "File not found: " << canonicalFull.string() << std::endl;
        return Response::fromErrorCode(404, m_serverConfig);
    }

    try
    {
        std::filesystem::remove(canonicalFull);
        std::cerr << "File deleted: " << canonicalFull.string() << std::endl;
        return Response::withStatus(204); // 204 No Content non ha body
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        std::cerr << "Error deleting file: " << e.what() << std::endl;
        return Response::fromErrorCode(500, m_serverConfig);
    }
}
