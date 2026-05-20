#include "../includes/CGI.hpp"

#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sstream>
#include <cstdlib>

std::string CGI::execute(
    const std::string &scriptPath,
    const std::string &interpreter,
    const std::string &method,
    const std::string &queryString,
    const std::string &body,
    const std::map<std::string, std::string> &headers
)
{
    (void)method;
    (void)queryString;
    (void)body;
    (void)headers;

    int pipefd[2];

    if (pipe(pipefd) < 0)
        return "CGI pipe error";

    pid_t pid = fork();

    if (pid < 0)
        return "CGI fork error";

    if (pid == 0)
    {
        dup2(pipefd[1], STDOUT_FILENO);

        close(pipefd[0]);
        close(pipefd[1]);

        char *argv[3];

        argv[0] = const_cast<char*>(interpreter.c_str());
        argv[1] = const_cast<char*>(scriptPath.c_str());
        argv[2] = NULL;

        execve(argv[0], argv, NULL);

        exit(1);
    }

    close(pipefd[1]);

    char buffer[1024];

    std::stringstream output;

    int bytes;

    while ((bytes = read(pipefd[0], buffer, 1023)) > 0)
    {
        buffer[bytes] = '\0';

        output << buffer;
    }

    close(pipefd[0]);

    waitpid(pid, NULL, 0);

    return output.str();
}