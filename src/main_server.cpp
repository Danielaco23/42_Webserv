#include "../includes/Server.hpp"
#include <signal.h>

int main()
{
    // prevent SIGPIPE from killing the process when writing to closed sockets
    signal(SIGPIPE, SIG_IGN);

    Server server(8080);

    server.initSocket();
    server.initVariables();
    server.startListening();

    while (true)
    {
        server.acceptConnection();
    }
    server.run();

    return 0;
}

// c++ -Wall -Wextra -Werror -std=c++98 -g3 -Iincludes main_server.cpp Server.cpp Client.cpp -o webserv
//  ./webserver
// entrar en local http://localhost:8080/
// http mero sencillo
// Ya chambe XD me pondre con los cliente o organizare el server mas visual

// revision si el puerto esta siendo usado
//lsof -i :8080