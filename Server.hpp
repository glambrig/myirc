#pragma once

#include "User.hpp"
#include "Commands.hpp"
#include "Channel.hpp"
#include <vector>
#include <stdlib.h>
#include <ctype.h>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netdb.h>
#include <poll.h>
#include <stdio.h>
#include <map>

class Server
{
private:
	std::vector<Channel>	_channels;
	std::vector<User>		_users;
	size_t					_port;
	std::string				sPort;
	std::string				sPassword;
	Commands				commands;
public:
	Server();
	Server(const Server& copy);
	Server(int ac, char **av);
	Server& operator=(const Server& rhs);
	~Server();

	std::vector<User>	getUsersFromServ();

	void	parseArgs(int ac, char **av);
	void	socketSetup(int &listenfd, struct sockaddr_in &servAddr);
	void	handlePollIn(struct pollfd	**pfdsArr, size_t pfdsArrLen, size_t i, int listenfd);

	int		parseIncomingMessage(const std::string buffer, const int i);

	void	run();

	bool	errToggle;	//If an error occurs in parseArgs(), errToggle==true, program doesn't run
};
