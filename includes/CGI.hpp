#ifndef CGI_HPP
#define CGI_HPP

#include <string>
#include <map>

class CGI
{
public:
    static std::string execute(
        const std::string& scriptPath,
        const std::string& interpreter,
        const std::string& method,
        const std::string& queryString,
        const std::string& body,
        const std::map<std::string, std::string>& headers);
};

#endif