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
	newCmd = "PRIVMSG";
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
	std::string reply = ":localhost " + err + "\r\n";

	if (send(user.socket, reply.c_str(), reply.size(), 0) < 0)
	{
		std::cout << "Failed to send reply to user " << user.username << std::endl;
		throw ("Critical error");
	}
	return (0);
}

int	Commands::nick(User& user, const std::string buff, std::vector<User> userList)
{
	if (buff.size() == 0)
	{
		std::string NONICKNAMEGIVEN = S_ERR_NONICKNAMEGIVEN + user.username + "NICK" +
			" :No nickname given";
		return (sendNumericReply(user, NONICKNAMEGIVEN));
	}

	const std::string forbidden = " ,*?!@";
	const std::string startForbidden = "$:#&~%+";

	if (buff.find(startForbidden.c_str(), 0, 1) != std::string::npos || buff.find(forbidden) != std::string::npos || buff.size() - 1 > 9)
	{
		// std::string test(S_ERR_ERRONEUSNICKNAME + ' ' + user.username + ' ' + buff + " :Erroneus nickname");
		std::string ERRONEUSNICKNAME;
		ERRONEUSNICKNAME.clear();
		ERRONEUSNICKNAME = S_ERR_ERRONEUSNICKNAME;
		ERRONEUSNICKNAME += ' ';
		if (user.username.empty())
			ERRONEUSNICKNAME += '*';
		else
			ERRONEUSNICKNAME += user.username;
		ERRONEUSNICKNAME +=  ' ';
		ERRONEUSNICKNAME += buff.substr(1, buff.size() - 3);
		ERRONEUSNICKNAME += " :Erroneus nickname";	//Actually spelled "erroneous" but ok
		std::cout << ERRONEUSNICKNAME << std::endl;
		return (sendNumericReply(user, ERRONEUSNICKNAME));
	}
	size_t endPos = buff.find("\r\n", 0);
	for (std::vector<User>::iterator it = userList.begin(); it != userList.end(); it++)
	{
		const std::string tempBuff = buff.substr(1, endPos - 1);
		if (it->nickname.empty() == true)
			break ;
		std::string tempNickName = it->nickname;
		for (size_t i = 0; i < tempBuff.size(); i++)
			toupper(tempBuff[i]);
		for (size_t i = 0; i < tempNickName.size(); i++)
			toupper(tempNickName[i]);
		if (tempBuff == tempNickName)// && tempNickName != user.nickname
		{
			const std::string NICKNAMEINUSE = S_ERR_NICKNAMEINUSE + user.username + " " + buff
				+ " :Nickname is already in use";
			return (sendNumericReply(user, NICKNAMEINUSE));
		}
	}
	user.nickname = buff.substr(1, buff.size() - 3);
	if (this->userCommand.empty() == false)
		Commands::user(user, this->userCommand);
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
			return (1);
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

int	Commands::user(User& user, std::string buff)
{
	if (user.nickname.empty())
	{
		userCommand = buff;
		return (0);
	}
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
	for (size_t i = 1; i < buff.size(); i++)
	{
		if (buff[i] == ' ')
		{
			user.username = buff.substr(1, i - 1);
			break ;
		}
	}

	// std::string welcome(S_RPL_WELCOME + ' ' + user.nickname + " :Welcome to my IRC server!");
	std::string welcome = S_RPL_WELCOME;
	welcome += ' ';
	if (user.nickname.empty() == false)
		welcome += user.nickname;	//doesn't yet exist at time of USER call if nick is invalid
	else
		welcome += '*';
	welcome += " :Welcome to my IRC server!";

	// std::string yourHost(S_RPL_YOURHOST + ' ' + user.nickname + " :Your host is roboserv, running version 1.0");
	std::string yourHost = S_RPL_YOURHOST;
	yourHost += " ";
	yourHost += user.nickname;
	yourHost += " :Your host is <servername>, running version 1.0";

	// std::string created(S_RPL_CREATED + ' ' + user.nickname + " :This server was created just now");
	std::string created = S_RPL_CREATED;
	created += " ";
	created += user.nickname;
	created += " :This server was created just now";

	sendNumericReply(user, welcome);
	sendNumericReply(user, yourHost);
	sendNumericReply(user, created);
	this->userCommand.clear();
	return (0);
}

/*
	Checks for leading # in front of channel name, and makes sure there are no spaces
	Remember that we have to subtract 2 from substr each time when there's a \r\n sequence
*/
int	joinParse(const std::string &buffer)
{
	if (buffer.empty() == true || buffer[0] != ' ' || buffer[1] != '#')
		return (-1);
	std::string buff = buffer.substr(2, buffer.size() - 4);
	for (size_t i = 0; i < buff.size(); i++)
	{
		if (!isalnum(buff[i]))
			return (-1);
	}
	return (0);	
}

int	Commands::join(User& user, const std::string buffer, std::vector<Channel> &channelList) const
{
	//Check if client CAN join the given channel
		//does channel exist
			//if so, does user have permission to join (invite only?)
		//otherwise create it, and add user to it
	if (joinParse(buffer) < 0)
	{
		std::string BADCHANMASK = S_ERR_BADCHANMASK;
		BADCHANMASK += ' ';
		BADCHANMASK += buffer.substr(1, buffer.size() - 3);
		BADCHANMASK += " :Bad Channel Mask";
		return (sendNumericReply(user, BADCHANMASK));
	}
	std::string buff = buffer.substr(1, buffer.size() - 3);	//removing the space and \r\n to leave only '#channelname'
	
	bool	channelExists = false;
	Channel chan;
	for (std::vector<Channel>::iterator it = channelList.begin(); it != channelList.end(); it++)
	{
		if (buff == (*it).getChanName())
		{
			channelExists = true;
			chan = *it;
			break ;
		}
	}
	if (channelExists == true)
	{
		for (std::vector<Channel>::iterator it = channelList.begin(); it != channelList.end(); it++)
		{
			if (buff == it->getChanName())
			{
				if (it->flags.inviteOnly == false)
				{
					//if there are other users, send JOIN to them
					if (it->getChanMembers().size() > 0)
					{
						std::vector<User> members = it->getChanMembers();
						std::string joinRelayMessage(':' + user.nickname + '!' + user.username + '@' + "localhost " + "JOIN " + buff + "\r\n");
						for (std::vector<User>::const_iterator subIter = members.begin(); subIter != members.end(); subIter++)
						{
							//sending "full client identifier" (nickname!username@hostname)
							send(subIter->socket, joinRelayMessage.c_str(), joinRelayMessage.size(), 0);
						}
					}
				}
				//join the channel
				it->addMember(user);
				break ;
			}
		}
	}
	else
	{
		Channel newChan;
		newChan.setChanName(buff);//.substr(1, buff.size() - 1)
		newChan.addMember(user);
		channelList.push_back(newChan);
	}
	std::string joinReply(":" + user.nickname + "!" + user.username + "@" + "localhost"
		+ " JOIN " + buff + "\r\n");
	send(user.socket, joinReply.c_str(), joinReply.size(), 0);
	return (0);
}

/*:<source> PRIVMSG <target> :<message>*/
int		Commands::privmsg(User& user, const std::string &buffer, const std::vector<Channel> channelList, const std::vector<User> userList) const
{
	//check if target is a user, or if it's a channel
		//if neither, error NOSUCHNICK i think
		//if user, send it
		//if channel, find out who's in it, and send to all of them
	(void)userList;

	if (buffer.empty() == true || buffer[0] != ' ')
	{
		std::string NORECIPIENT = S_ERR_NORECIPIENT;
		NORECIPIENT += ' ';
		NORECIPIENT += user.nickname;
		NORECIPIENT += " :No recipient given (PRIVMSG)";
		return (sendNumericReply(user, NORECIPIENT));
	}
	std::string buff = buffer.substr(1, buffer.size() - 1);
	std::string target;
	std::string message;
	for (size_t i = 0; i < buff.size(); i++)
	{
		if (buff[i] == ' ')
		{
			target = buff.substr(0, i);
			message = buff.substr(i + 2, buff.size() - i);
			break ;
		}
	}
	for (std::vector<Channel>::const_iterator it = channelList.begin(); it != channelList.end(); it++)
	{
		if (target == it->getChanName())
		{
			std::vector<User> chanMembers = it->getChanMembers();
			//Full client identifier + PRIVMSG + #channel + :message
			std::string fullMsg(':' + user.nickname + '!' + user.username + "@localhost " + "PRIVMSG" + buffer);
			for (size_t i = 0; i < chanMembers.size(); i++)
			{
				if (chanMembers[i].nickname != user.nickname)
					send(chanMembers[i].socket, fullMsg.c_str(), fullMsg.size(), 0);
			}
			break ;
		}
	}
	return (0);
}
