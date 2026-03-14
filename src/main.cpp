
#include "webserv.hpp"

void	print_cout(std::string printable)
{
	std::cout << printable << std::endl;
}

void	print_cerr(std::string printable)
{
	const std::string color_R = "\033[0;31m";
	const std::string color_NC = "\033[0m";
	std::cerr << color_R << printable << color_NC << std::endl;
}

int main(int argc, char const *argv[])
{
	if (argc != 2)
		return (print_cerr(ARG_ERR), 1);
	print_cout(argv[1]);
	return (0);
}

