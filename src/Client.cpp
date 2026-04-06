#include "../includes/Client.hpp"

Client::Client(int fd) : fd(fd), state(READING) {}