#include "Commands.hpp"
#include "Server.hpp"

Commands::Commands()
{
	std::string newCmd;

	newCmd = "JOIN";
	cmdList.push_back(newCmd);
	newCmd = "NICK";
	cmdList.push_back(newCmd);
	newCmd = "USER";
	cmdList.push_back(newCmd);
	hasRegistered = false;
}

bool	Commands::isValidCommand(const std::string &s) const
{
	for (std::list<std::string>::const_iterator it = cmdList.begin(); it != cmdList.end(); it++)
	{
		if (s == *it)
			return (true);
	}
	return (false);
}

//For different error key/value pairs, see #define in .hpp
int	Commands::sendNumericReply(const User &user, const std::string& err) const
{
	std::string reply = ":localhost " + err;

	if (send(user.socket, reply.c_str(), reply.size(), 0) < 0)
	{
		std::cout << "Failed to send reply to user " << user.username << std::endl;
		throw ("Critical error");
	}
	return (0);
}

int	Commands::nick(User& user, const std::string buff, std::vector<User> userList) const
{
	if (buff.size() == 0)
	{
		std::string NONICKNAMEGIVEN = S_ERR_NONICKNAMEGIVEN + user.username + "NICK" +
			" :No nickname given";
		return (sendNumericReply(user, NONICKNAMEGIVEN));
	}

	std::string forbidden = " ,*?!@";
	std::string startForbidden = "$:#&~%+";

	if (buff.find(startForbidden.c_str(), 0, 1) != std::string::npos || buff.find(forbidden) != std::string::npos)
	{
		std::string ERRONEUSNICKNAME = S_ERR_ERRONEUSNICKNAME + user.username + " "
			+ buff + " :Erroneus nickname";	//Actually spelled "erroneous" but ok
		return (sendNumericReply(user, ERRONEUSNICKNAME));
	}
	for (std::vector<User>::iterator it = userList.begin(); it != userList.end(); it++)
	{
		std::string tempBuff = buff;
		std::string tempNickName = (*it).nickname;
		for (size_t i = 0; i < tempBuff.size(); i++)
			toupper(tempBuff[i]);
		for (size_t i = 0; i < tempNickName.size(); i++)
			toupper(tempNickName[i]);
		if (tempBuff == tempNickName && tempNickName != user.nickname)
		{
			std::string NICKNAMEINUSE = S_ERR_NICKNAMEINUSE + user.username + " " + buff
				+ " :Nickname is already in use";
			return (sendNumericReply(user, NICKNAMEINUSE));
		}
	}
	user.nickname = buff;
	return (0);
}

/*
	Syntax: USER <username> (ignore) (ignore) :<realname>
	This function verifies that the command was sent with correct syntax.
	'buff' is a substring containing everything after "USER".
*/
int	Commands::parseUserBuff(const std::string &buff) const
{
	if (buff[0] != ' ')
	{
		return (1);
	}
	
	//Making sure there are exactly 4 spaces before :<realname>
	for (int i = 0, spaceCount = 0; buff[i] && buff[i] != ':'; i++)
	{
		if (buff[i] == ' ')// && buff[i - 1] != ' '
			spaceCount++;
		if (spaceCount > 4)
		{
			return (1);
		}
	}
	
	//Username MUST have a length of at least 1
	for (int i = 1, count = 0; buff[count]; i++)
	{
		while (buff[i + count] && buff[i + count] != ' ')
			count++;
		if (count < 1)
			return (1);
		else
			return (0);
	}
	return (0);
}

int	Commands::user(User& user, const std::string buff) const
{
	if (parseUserBuff(buff) == 1)
	{
		std::string NEEDMOREPARAMS = "* :Not enough parameters";
		return (sendNumericReply(user, NEEDMOREPARAMS));
	}
	if (hasRegistered == true)
	{
		std::string ALREADYREGISTERED(S_ERR_ALREADYREGISTERED + ' ' + user.username + " USER : You may not reregister");
		return (sendNumericReply(user, ALREADYREGISTERED));
	}
	if (buff.size() == 0)
	{
		std::string NONICKNAMEGIVEN('*' + " USER :Not enough parameters");
		return (sendNumericReply(user, NONICKNAMEGIVEN));		
	}

	std::string welcome = S_RPL_WELCOME;
	welcome += " ";
	welcome += user.nickname;
	welcome += " :Welcome to my IRC server!";

	std::string yourHost = S_RPL_YOURHOST;
	yourHost += " ";
	yourHost += user.nickname;
	yourHost += " :Your host is <servername>, running version 1.0";

	std::string created = S_RPL_CREATED;
	created += " ";
	created += user.nickname;
	created += " :This server was created just now";

	sendNumericReply(user, welcome);
	sendNumericReply(user, yourHost);
	sendNumericReply(user, created);
	std::cout << "Welcome message sent." << std::endl;
	return (0);
}

int	Commands::join(User& user, const std::string buff, std::vector<User> userList) const
{
	(void)user;
	(void)buff;
	(void)userList;

	//Check if client CAN join the given channel
	return (0);
}
