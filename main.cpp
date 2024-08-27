#include "Server.hpp"
#include <signal.h>

void	ft_bzero(void *loc, size_t n)
{
	unsigned int	i;

	i = 0;
	while (i < n)
	{
		*((char *)loc + i) = '\0';
		i++;
	}
}

bool Server::_signal = false;

void	sighandler(int sig)
{
	(void)sig;
	std::cout << "Signal sent, server shutting down." << std::endl;
	Server::_signal = true;
}

int	main(int ac, char **av)
{
	Server	server(ac, av);
	struct sigaction	sa;

	if (server.errToggle == true)
		return (1);
	// memset(&sa, 0, sizeof(sa));
	ft_bzero(&sa, sizeof(sa));
	sa.sa_handler = &sighandler;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGQUIT, &sa, NULL);
	server.run();
	return (0);
}