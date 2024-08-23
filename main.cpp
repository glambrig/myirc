#include "Server.hpp"
#include <signal.h>

int	main(int ac, char **av)
{
	Server	server(ac, av);

	// signal(SIGQUIT, SIG_IGN);
	if (server.errToggle == true)
		return (1);
	server.run();
	return (0);
}