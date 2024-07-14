#include "Server.hpp"
#define MAX_CLIENTS 10

Server::Server()
{
	// pfdsArr = new struct pollfd[MAX_CLIENTS];	//can be alloc'd and freed in constructor/destructor
}

Server::Server(const Server& copy)
{
	*this = copy;
}

Server::Server(int ac, char **av)
{
	errToggle = false;
	try
	{
		parseArgs(ac, av);
	}
	catch (const char *e)
	{
		std::cout << e << std::endl;
		errToggle = true;
	}
}

Server& Server::operator=(const Server& rhs)
{
	if (&rhs != this)
		*this = rhs;
	return (*this);
}

Server::~Server()
{
	// delete[] pfdsArr;
}

std::vector<User>	Server::getUsersFromServ()
{
	return (_users);
}

void	Server::parseArgs(int ac, char **av)
{
	if (ac != 3)
		throw ("Server::parseArgs::Error: Invalid number of arguments");
	sPort = av[1];
	for (size_t i = 0; i < sPort.length(); i++)
	{
		if (isdigit(sPort[i]) == 0)
			throw ("Server::parseArgs::Error: Port is not a number");
	}
	_port = atoi(sPort.c_str());
	if (sPort[0] == '-')
		throw ("Server::parseArgs::Error: Negative port is invalid.");
	if (_port < 1023)
		throw ("Server::parseArgs::Error: Port is \"well known\". (Use 1024-65534)");
	if (_port > 65534)
		throw ("Server::parseArgs::Error: Port > 65534.");
	this->sPassword = av[2];
}

void	Server::socketSetup(int &listenfd, struct sockaddr_in &servAddr)
{
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	int one = 1;

	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int)) < 0)
		throw ("Server::socketSetup::Sock option error");
	if (listenfd < 0)
		throw ("Server::socketSetup::Error creating socket.");
	servAddr.sin_family = AF_INET;	//Expecting an internet address (ip)
	servAddr.sin_port = htons(_port);	//Incoming connections on port _port (specified by user)
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);	//Accept connections from any IP address
	if (bind(listenfd, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0)
		throw ("Server::socketSetup::Error binding address to socket.");
	if (listen(listenfd, MAX_CLIENTS) < 0)
		throw ("Server::socketSetup::Error listening for connections.");
}

int	noPass(User& user)
{
	// Commands commands;
	
	std::string error(":localhost * ERROR :No password provided\r\n");
	send(user.socket, error.c_str(), error.size(), 0);
	close(user.socket);
	// commands.quit(user);
	return (-1);
}

int Server::parseIncomingMessage(const std::string buff, const int i)
{
	User& 			user = _users[i];
	static Commands	commands;

	if (buff.length() < 4)
		return (-1);
	std::string substr = buff.substr(0, 4);
	
	static bool passSent = false;
	if (substr == "PASS")
	{
		passSent = true;
		return (commands.pass(user, buff.substr(4, buff.size() - 4), this->sPassword));
	}
 	if (substr == "NICK")
	{
		if (passSent == false)
			return (noPass(user));
		return (commands.nick(user, buff.substr(4, buff.size() - 4), this->_users));
	}
	if (substr == "USER")
	{
		if (passSent == false)
			return (noPass(user));
		return (commands.user(user, buff.substr(4, buff.size() - 4)));
	}
	if (substr == "JOIN")
	{
		if (passSent == false)
			return (noPass(user));
		return (commands.join(user, buff.substr(4, buff.size() - 4), this->_channels));
	}
	if (buff.substr(0, 5) == "TOPIC")
	{
		if (passSent == false)
			return (noPass(user));
		return (commands.topic(user, buff.substr(5, buff.size() - 5), this->_channels));
	}
	if (buff.substr(0, 7) == "PRIVMSG")
	{
		if (passSent == false)
			return (noPass(user));
		return (commands.privmsg(user, buff.substr(7, buff.size() - 7), this->_channels, this->_users));
	}
	return (-1);
}

/*
	XChat client sends NICK and User in a single call to send()
	We need the commands separated in order to handle them properly.
*/
std::vector<std::string>	splitRecvRes(std::string buff)
{
	std::vector<std::string> res;

	size_t pos = buff.find("\r\n");
	if (pos != std::string::npos)
	{
		res.push_back(buff.substr(0, pos + 2));
		buff = buff.substr(pos + 2, buff.size() - pos + 2);
	}
	else
		return (res);
	size_t lastfind = 0;
	while ((pos = buff.find("\r\n", lastfind)) != std::string::npos)
	{
		std::string temp = buff.substr(lastfind, pos - lastfind + 2);
		res.push_back(temp);
		lastfind = pos + 2;
	}
	return (res);
}

// std::vector<std::string>	splitRecvRes(const std::string &buff)
// {
// 	std::vector<std::string> res;
// 	int count = 0;
// 	int pos[100];

// 	for (size_t i = 0, k = 0; i < buff.length() - 1;)
// 	{
// 		if (buff[i] == '\r' && buff[i + 1] == '\n')
// 		{
// 			count++;
// 			pos[k++] = i + 2;
// 			i += 2;
// 			continue ;
// 		}
// 		i++;
// 	}
// 	for (int i = 0; i < count; i++)
// 	{
// 		if (i == 0)
// 		{
// 			std::string temp = buff.substr(0, pos[0]);
// 			res.push_back(temp);
// 		}
// 		else
// 		{
// 			std::string temp = buff.substr(pos[i - 1], pos[i]);
// 			res.push_back(temp);
// 		}
// 	}
// 	return (res);
// }

void	Server::handlePollIn(struct pollfd	**pfdsArr, size_t pfdsArrLen, size_t i, int listenfd)
{
	char	buff[512];

	if ((*pfdsArr)[i].fd == listenfd)
	{
		//Set up incoming connection
		std::cout << "Incoming connection" << std::endl;
		int clientfd = accept(listenfd, NULL, NULL);
		if (clientfd < 0)
			throw ("Error accepting connection.");

		struct pollfd temp;
		temp.fd = clientfd;
		temp.events = POLLIN;

		if (pfdsArrLen < MAX_CLIENTS)
		{
			std::cout << pfdsArrLen << std::endl;
			(*pfdsArr)[pfdsArrLen] = temp;
			User	newUsr;
			newUsr.socket = temp.fd;
			_users.push_back(newUsr);
		}
		else
		{
			std::cout << "Too many clients." << std::endl;
			for (int k = 0; k < MAX_CLIENTS; k++)
				close((*pfdsArr)[k].fd);
			return ;
		}
	}
	else
	{
		int recvRes = recv((*pfdsArr)[i].fd, &buff, 512, 0);
		if (recvRes <= 0)
		{
			close((*pfdsArr)[i].fd);
			// reallocArr(pfdsArr, i);	//need to write this. frees the space taken up by the disconnected client, and realloc's the array
			if (recvRes == 0)
			{
				// _users[i].setisConnected(false);
				std::cout << "Client disconnected" << std::endl;
			}
			else if (recvRes < 0)
				throw ("Error receiving from fd");
		}
		else
		{
			std::string strBuff = buff;
			while (strBuff.find("\r\n") == std::string::npos && strBuff.size() < 512)
				recv((*pfdsArr)[i].fd, &buff, 512, 0);
			
			std::cout << "Server received message: " << buff << std::endl;////////

			std::vector<std::string> splitBuff = splitRecvRes(buff);
			for (std::vector<std::string>::iterator it = splitBuff.begin(); it != splitBuff.end(); it++)
				(void)parseIncomingMessage(*it, i - 1);
		}
	}
	for (size_t k = 0; k < 512; k++)
		buff[k] = '\0';
}


void	Server::run()
{
	int					listenfd;	//Server accepts connections on this socket
	struct sockaddr_in	servAddr;

	try
	{
		socketSetup(listenfd, servAddr);
	}
	catch (const char *e)
	{
		std::cout << e << std::endl;
		return ;
	}

	struct pollfd	*pfdsArr = new struct pollfd[MAX_CLIENTS];	//can be alloc'd and freed in constructor/destructor
	struct pollfd	pfd;
	pfd.fd = listenfd;
	pfd.events = POLLIN | POLLOUT;	//I don't think POLLOUT is ever received
	pfdsArr[0] = pfd;

	for (size_t i = 1; i < MAX_CLIENTS; i++)
		pfdsArr[i].fd = 0;

	std::cout << "Server is running." << std::endl;
	try
	{
		while (1)
		{
			size_t pfdsArrLen = 0;
			for (size_t i = 0; i < MAX_CLIENTS; i++)
			{
				if (pfdsArr[i].fd == 0)
					break ;
				pfdsArrLen++;
			}
			int pollreturn = poll(pfdsArr, pfdsArrLen, -1);
			if (pollreturn < 0)
				throw ("Poll returned negative value");

			for (size_t i = 0; i < pfdsArrLen; i++)
			{
				if (pfdsArr[i].revents & POLLIN)
					handlePollIn(&pfdsArr, pfdsArrLen, i, listenfd);
				if (pfdsArr[i].revents & POLLOUT)// handlePollOut();//////
				{//////
					std::cout << "Something on POLLOUT." << std::endl;//////
					send(pfdsArr[i].fd, "hi\n", 3, 0);//////
					continue;//////
				}//////
			}
		}
	}
	catch (const char *e)
	{
		std::cout << e << std::endl;
		delete[] pfdsArr;
		return ;
	}
}
