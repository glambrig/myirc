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

void	Server::clearClient(int fd)
{
	for (std::vector<User>::iterator it = this->_users.begin(); it != this->_users.end(); it++)
	{
		if (it->socket == fd)
		{
			std::string leavingMsg(" :Leaving");
			Commands::quit(*it, leavingMsg, this->_users, this->pfdsArr);
		}
	}
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
	if (fcntl(listenfd, F_SETFL, O_NONBLOCK) < 0)
		throw ("Server::socketSetup::fcntl() failed.");
	if (bind(listenfd, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0)
		throw ("Server::socketSetup::Error binding address to socket.");
	if (listen(listenfd, MAX_CLIENTS) < 0)
		throw ("Server::socketSetup::Error listening for connections.");
}

int	noPass(User& user)
{
	std::string error(":localhost * ERROR :No password provided\r\n");
	send(user.socket, error.c_str(), error.size(), 0);
	close(user.socket);
	return (-1);
}

/*This function NEEDS to be revised. Parsing is pretty bad. Ex. passing "MODES" will go into MODE command*/
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
	if (substr == "MODE")
	{
		if (passSent == false)
			return (noPass(user));	
		return (commands.mode(user, buff.substr(4, buff.size() - 4), this->_channels));
	}
	if (substr == "KICK")
	{
		if (passSent == false)
			return (noPass(user));
		return (commands.kick(user, this->_channels, buff.substr(4, buff.size() - 4)));
	}
	if (substr == "PART")
	{
		if (passSent == false)
			return (noPass(user));
		return (commands.part(user, buff.substr(4, buff.size() - 4), this->_channels));
	}
	if (substr == "QUIT")
	{
		if (passSent == false)
			return (noPass(user));
		return (commands.quit(user, buff.substr(4, buff.size() - 4), this->_users, this->pfdsArr));
	}
	if (buff.substr(0, 5) == "TOPIC")
	{
		if (passSent == false)
			return (noPass(user));
		return (commands.topic(user, buff.substr(5, buff.size() - 5), this->_channels));
	}
	if (buff.substr(0, 6) == "INVITE")
	{
		if (passSent == false)
			return (noPass(user));
		return (commands.invite(user, buff.substr(6, buff.size() - 6), this->_channels, this->_users));
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
	XChat client sends NICK and USER in a single call to send()
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

void	Server::handlePollIn(size_t pfdsArrLen, size_t i, int listenfd)
{
	char	buff[512];

	if (pfdsArr[i].fd == listenfd)
	{
		//Set up incoming connection
		std::cout << "Incoming connection" << std::endl;
		int clientfd = accept(listenfd, NULL, NULL);
		if (clientfd < 0)
			throw ("Error accepting connection.");
		// if (fcntl(clientfd, F_SETFL, O_NONBLOCK) < 0)
		// 	throw ("fcntl() failed.");

		struct pollfd temp;
		temp.fd = clientfd;
		temp.events = POLLIN;

		if (pfdsArrLen < MAX_CLIENTS)
		{
			pfdsArr.push_back(temp);
			User	newUsr;
			newUsr.socket = temp.fd;
			_users.push_back(newUsr);
		}
		else
		{
			std::cout << "Too many clients." << std::endl;
			return ;
		}
	}
	else
	{
		int recvRes = recv(pfdsArr[i].fd, &buff, 512, 0);
		if (recvRes <= 0)
		{
			close(pfdsArr[i].fd);
			if (recvRes == 0)
				std::cout << "Client disconnected" << std::endl;
			else if (recvRes < 0)
				throw ("Error receiving from fd");
		}
		else
		{
			std::string strBuff(buff);
			static std::vector<std::pair<int, std::string> > receivedCmds;
			if (receivedCmds.empty())
			{
				std::pair<int, std::string> temp;
				temp.first = pfdsArr[i].fd;
				temp.second = strBuff;
				receivedCmds.push_back(temp);
			}
			else
			{
				for (std::vector<std::pair<int, std::string> >::iterator it = receivedCmds.begin(); it != receivedCmds.end(); it++)
				{
					if (pfdsArr[i].fd == it->first)
					{
						it->second.clear();
						it->second = strBuff;
						break ;
					}
					if (it + 1 == receivedCmds.end())
					{
						std::pair<int, std::string> temp;
						temp.first = pfdsArr[i].fd;
						temp.second = strBuff;
						receivedCmds.push_back(temp);
					}
				}
			}

			////
			while (strBuff.find("\r\n") == std::string::npos && strBuff.size() < 512)
			{
				recv(pfdsArr[i].fd, &buff, 512, 0);
				strBuff += buff;
			}
			
			std::cout << "Server received message: " << strBuff << std::endl;////////

			std::vector<std::string> splitBuff = splitRecvRes(strBuff);
			for (std::vector<std::string>::iterator it = splitBuff.begin(); it != splitBuff.end(); it++)
				(void)parseIncomingMessage(*it, i - 1);
		}
	}
	ft_bzero(&buff, 512);
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

	// struct pollfd	*pfdsArr = new struct pollfd[MAX_CLIENTS];	//can be alloc'd and freed in constructor/destructor
	struct pollfd	pfd;
	pfd.fd = listenfd;
	pfd.events = POLLIN | POLLPRI | POLLRDHUP | POLLERR | POLLHUP;
	// pfdsArr[0] = pfd;
	pfdsArr.push_back(pfd);

	// for (size_t i = 1; i < MAX_CLIENTS; i++)
	// 	pfdsArr[i].fd = 0;

	std::cout << "Server is running." << std::endl;
	try
	{
		while (Server::_signal == false)
		{
			size_t pfdsArrLen = pfdsArr.size();
			int pollreturn = poll(&pfdsArr[0], pfdsArrLen, -1);
			if (pollreturn < 0)
				throw ("Poll returned negative value");

			for (size_t i = 0; i < pfdsArrLen; i++)
			{
				if (pfdsArr[i].revents & POLLIN)
					handlePollIn(pfdsArrLen, i, listenfd);

				if (pfdsArr[i].revents & POLLPRI)
					std::cout << "POLLPRI called" << std::endl;
				if (pfdsArr[i].revents & POLLERR)
					std::cout << "POLLERR called" << std::endl;
				if (pfdsArr[i].revents & POLLRDHUP)
					std::cout << "POLLRDHUP called" << std::endl;
				if (pfdsArr[i].revents & POLLHUP)
					std::cout << "POLLHUP called" << std::endl;
				//TODO:
				/*Add all the POLLIN, POLLERR... options to see which one triggers when nc client ctrl+z'd
					and deal with it*/
			}
		}
		for (std::vector<User>::iterator it = this->_users.begin(); it != this->_users.end(); it++)
			clearClient(it->socket);
	}
	catch (const char *e)
	{
		std::cout << e << std::endl;
		return ;
	}
}
