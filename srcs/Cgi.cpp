#include "headers.hpp"


static void setenv_str(const char* k, const std::string& v) {
    setenv(k, v.c_str(), 1);
}
// Helper: read with timeout
static bool readWithTimeout(int fd, std::string &out, int timeoutSec) {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    struct timeval tv;
    tv.tv_sec = timeoutSec;
    tv.tv_usec = 0;

    char buf[4096];
    while (true) {
        fd_set rfds = fds;
        int ret = select(fd + 1, &rfds, NULL, NULL, &tv);
        if (ret == 0) return false;  // timeout
        if (ret < 0) {
            if (errno == EINTR) continue;
            return false;
        }

        if (FD_ISSET(fd, &rfds)) {
            ssize_t n = read(fd, buf, sizeof(buf));
            if (n <= 0) break;
            out.append(buf, n);
        } else {
            break;
        }
    }
    return true;
}

CgiResult runCgi(const Request& req,
                 const std::string& scriptPath,
                 const std::string& interpreter) {
    CgiResult r;

    int inPipe[2];
    int outPipe[2];
    if (pipe(inPipe) < 0 || pipe(outPipe) < 0) {
        r.status = 500; r.body = "pipe() failed\n";
        return r;
    }

    pid_t pid = fork();
    if (pid < 0) {
        r.status = 500; r.body = "fork() failed\n";
        return r;
    }

    if (pid == 0) {
        dup2(inPipe[0], STDIN_FILENO);
        dup2(outPipe[1], STDOUT_FILENO);

        close(inPipe[0]);
        close(inPipe[1]);
        close(outPipe[0]);
        close(outPipe[1]);

        setenv("GATEWAY_INTERFACE", "CGI/1.1", 1);
        setenv_str("REQUEST_METHOD", req.getMethod());
        setenv_str("SCRIPT_FILENAME", scriptPath);
        setenv_str("QUERY_STRING", req.getQuery());
        setenv_str("CONTENT_TYPE", req.getHeader("Content-Type"));
        setenv_str("CONTENT_LENGTH", req.getHeader("Content-Length"));
        setenv("SERVER_PROTOCOL", "HTTP/1.1", 1);
        setenv_str("HTTP_HOST", req.getHost());

        std::vector<char*> argv;
        if (!interpreter.empty()) {
            argv.push_back(const_cast<char*>(interpreter.c_str()));
            argv.push_back(const_cast<char*>(scriptPath.c_str()));
        } else {
            argv.push_back(const_cast<char*>(scriptPath.c_str()));
        }
        argv.push_back(nullptr);

        execvp(argv[0], argv.data());
        const char* msg = "execvp failed\n";
        write(STDOUT_FILENO, msg, strlen(msg));
        _exit(127);
    }

    // === Parent process ===
    close(inPipe[0]);
    close(outPipe[1]);

    // Send POST body if any
    const std::string& body = req.getBody();
    if (!body.empty())
        write(inPipe[1], body.c_str(), body.size());
    close(inPipe[1]);

    std::string out;
bool ok = readWithTimeout(outPipe[0], out, 3);  // 3s timeout
close(outPipe[0]);

int status = 0;
waitpid(pid, &status, 0);

if (!ok) {
    kill(pid, SIGKILL);
    waitpid(pid, NULL, 0);
    r.status = 504;
    r.body = "<h1>504 Gateway Timeout (CGI read timeout)</h1>";
    return r;
}

if (WIFSIGNALED(status)) {
    r.status = 502;
    r.body = "<h1>502 Bad Gateway (CGI killed by signal)</h1>";
    return r;
}

if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
    r.status = 500;
    r.body = "<h1>500 Internal Server Error (CGI failed)</h1>";
    return r;
}


    // === Normal output parsing ===
    size_t sep = out.find("\r\n\r\n");
    if (sep == std::string::npos)
        sep = out.find("\n\n");

    std::string head = "";
    std::string bodyOut = out;

    if (sep != std::string::npos) {
        head = out.substr(0, sep);
        bodyOut = out.substr(sep + ((out.find("\r\n\r\n") != std::string::npos) ? 4 : 2));
    }

    int httpStatus = 200;
    std::map<std::string, std::string> headers;

    std::istringstream iss(head);
    std::string line;
    while (std::getline(iss, line)) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (line.empty()) continue;

        size_t col = line.find(':');
        if (col == std::string::npos) {
            if (line.rfind("Status", 0) == 0) {
                size_t sp = line.find(' ');
                if (sp != std::string::npos)
                    httpStatus = std::atoi(line.c_str() + sp + 1);
            }
            continue;
        }

        std::string key = line.substr(0, col);
        std::string val = line.substr(col + 1);
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        while (!val.empty() && (val.front() == ' ' || val.front() == '\t'))
            val.erase(0, 1);

        headers[key] = val;
    }

    r.status = httpStatus;
    r.headers = headers;
    r.body = bodyOut;
    return r;
}
