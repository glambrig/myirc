#include "Server.hpp"

int	main(int ac, char **av)
{
	Server	server(ac, av);

	if (server.errToggle == true)
		return (1);
	server.run();
	return (0);
}