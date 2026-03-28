#include "../includes/Server.hpp"
#include <signal.h>

int main()
{
    // prevent SIGPIPE from killing the process when writing to closed sockets
    signal(SIGPIPE, SIG_IGN);

    Server server(8080);

    server.initSocket();
    server.startListening();

    while (true)
    {
        server.acceptConnection();
    }

    return 0;
}

// g++ -Wall -Wextra -Werror -std=c++98 src/main_server.cpp src/Server.cpp -o webserv
//  ./webserver
// entrar en local http://localhost:8080/
// http mero sencillo
// Ya chambe XD me pondre con los cliente o organizare el server mas visual