#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <string>
#include <map>

//http parse temporal necesito algo con lo q chambear
class Request
{
    public:

        std::string method;
        std::string path;
        std::string queryString;
        std::string body;

        std::map<std::string, std::string> headers;
};

#endif