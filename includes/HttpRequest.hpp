#ifndef HTTPREQUESTREQUEST_HPP
#define HTTPREQUESTREQUEST_HPP

#include <string>
#include <map>

# define GREEN "\033[0;32m"
# define WHITE "\033[37m"
# define RED "\033[0;31m\033[1m"
# define ORANGE "\001\033[38;5;208m\002"
# define BLUE "\033[0;34m"
# define PURPLE "\033[0;35m"
# define CYAN "\033[0;36m"
# define YELLOW  "\x1b[33m"
# define ROSE    "\x1B[38;2;255;151;203m"
# define LIGHT_BLUE   "\x1B[38;2;53;149;240m"
# define LIGHT_GREEN  "\x1B[38;2;17;245;120m"
# define GRAY    "\x1B[38;2;176;174;174m"
# define NC "\033[0m"

struct HttpRequest
{
    std::string _method;
    std::string _file_path;
    std::string _req;
    std::string _request_id;
    std::string _path;
    std::string _version;
    std::string _query_string;   // 👈 IMPORTANTE (te falta en errores)
    std::string _body;           // 👈 IMPORTANTE
    std::string _www_root;
    int _client_fd;
    size_t _maxBodySize;
};

#endif
