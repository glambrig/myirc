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
	newCmd = "TOPIC";
	cmdList.push_back(newCmd);
	newCmd = "PRIVMSG";
	cmdList.push_back(newCmd);
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
int	Commands::sendNumericReply(const User &user, const std::string& err)
{
	std::string reply = ":localhost " + err + "\r\n";

	if (send(user.socket, reply.c_str(), reply.size(), 0) < 0)
	{
		std::cout << "Failed to send reply to user " << user.username << std::endl;
		throw ("Critical error");
	}
	return (0);
}

int		Commands::pass(User& user, const std::string& buffer, const std::string& password) const
{
	std::string buff(buffer.substr(1, buffer.size() - 3));

	if (buff.find("CAP LS") != std::string::npos)	//Silly. Ignores CAP LS message from hexchat
		return (0);
	if (buff != password)
	{
		std::string PASSMISMATCH(S_ERR_PASSWDMISMATCH);
		PASSMISMATCH += " * :Password incorrect";
		sendNumericReply(user, PASSMISMATCH);
		return (-1);
	}
	else
	{
		std::cout << "Password is correct! :)" << std::endl;
	}
	return (0);
}

int	Commands::nick(User* user, const std::string buff, std::vector<User*> &userList)
{
	if (buff.size() == 0)
	{
		std::string NONICKNAMEGIVEN = S_ERR_NONICKNAMEGIVEN + user->username + "NICK" +
			" :No nickname given";
		return (sendNumericReply(*user, NONICKNAMEGIVEN));
	}

	const std::string forbidden = " ,*?!@";
	const std::string startForbidden = "$:#&~%+";

	if (buff.find(startForbidden.c_str(), 0, 1) != std::string::npos || buff.find(forbidden) != std::string::npos || buff.size() - 1 > 9)
	{
		std::string ERRONEUSNICKNAME(S_ERR_ERRONEUSNICKNAME);
		ERRONEUSNICKNAME += ' ';
		if (user->username.empty())
			ERRONEUSNICKNAME += '*';
		else
			ERRONEUSNICKNAME += user->username;
		ERRONEUSNICKNAME +=  " " + buff.substr(1, buff.size() - 3) + " :Erroneus nickname";
		return (sendNumericReply(*user, ERRONEUSNICKNAME));
	}
	size_t endPos = buff.find("\r\n", 0);	//careful: not checking for std::string::npos!
	std::string oldNickName;
	for (std::vector<User*>::iterator it = userList.begin(); it != userList.end(); it++)
	{
		std::string tempBuff = buff.substr(1, endPos - 1);
		if ((*it)->nickname.empty() == true)
			break ;
		oldNickName = (*it)->nickname;
		std::string tempNickName = oldNickName;
		for (size_t i = 0; i < tempBuff.size(); i++)
			tempBuff[i] = toupper(tempBuff[i]);
		for (size_t i = 0; i < oldNickName.size(); i++)
			tempNickName[i] = toupper(oldNickName[i]);

		if (tempBuff == tempNickName)// && tempNickName != user.nickname
		{
			std::string NICKNAMEINUSE(S_ERR_NICKNAMEINUSE);
			NICKNAMEINUSE += ' ';
			if (user->username.empty())
				NICKNAMEINUSE += '*';
			else
				NICKNAMEINUSE += user->username;
			NICKNAMEINUSE += ' ';
			NICKNAMEINUSE += oldNickName;
			NICKNAMEINUSE += " :Nickname is already in use";
			return (sendNumericReply(*user, NICKNAMEINUSE));
		}
	}
	if (user->nickname.empty())
		oldNickName = '*';
	else
		oldNickName = user->nickname;
	user->nickname = buff.substr(1, buff.size() - 3);
	std::string nickreply(':' + oldNickName + " NICK " + user->nickname + "\r\n");
	for (std::vector<User*>::iterator it = userList.begin(); it != userList.end(); it++)
	{
		if ((*it)->hasRegistered == true)
			send((*it)->socket, nickreply.c_str(), nickreply.size(), 0);
	}
	if (this->userCommand.empty() == false)
	{
		Commands::user(*user, this->userCommand);
		user->hasRegistered = true;
	}
	return (0);
}

/*
	Syntax: USER <username> (ignore) (ignore) :<realname>
	This function verifies that the command was sent with correct syntax.
	'buff' is a substring containing everything after "USER".
*/
int	Commands::parseUserBuff(const std::string &buffer) const
{
	int a;
	std::string buff(buffer.substr(0, buffer.size() - 2));

	if (buff[0] != ' ' || buff.find(':', 0) == std::string::npos)
		return (-1);
	a = buff.find(':', 0) + 1;
	for (int i = a; buff[i]; i++)
	{
		if (buff[i] == ' ')
			return (-1);
	}
	if (buff[a] == '\0' || std::isalnum(buff[a]) == false)
		return (-1);
	a = buff.find(':', 0) - 1;
	if (buff[a] != ' ')
		return (-1);

	int spaceCount = 0;
	//Making sure there are exactly 4 spaces before :<realname>
	for (int i = 0; buff[i] && buff[i] != ':'; i++)
	{
		size_t pos = buff.find_first_not_of(' ', i);
		if (buff[i] == ' ' && (pos != std::string::npos && buff[pos] != '\0'))
			spaceCount++;
	}
	if (spaceCount != 4)
		return (-1);

	//Username MUST have a length of at least 1
	for (int i = 1, count = 0; buff[count]; i++)
	{
		while (buff[i + count] && buff[i + count] != ' ')
			count++;
		if (count < 1)
			return (-1);
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
	if (parseUserBuff(buff) < 0 || buff.size() == 0)
	{
		std::string NEEDMOREPARAMS = "* :Not enough parameters";
		return (sendNumericReply(user, NEEDMOREPARAMS));
	}
	if (user.hasRegistered == true)
	{
		std::string ALREADYREGISTERED(S_ERR_ALREADYREGISTERED);
		ALREADYREGISTERED += " " + user.username + " USER : You may not reregister";
		return (sendNumericReply(user, ALREADYREGISTERED));
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
	if (!user.nickname.empty())
		user.hasRegistered = true;
	return (0);
}

/*
	Checks for leading # in front of channel name, and makes sure there are no spaces
	Remember that we have to subtract 2 from substr each time when there's a \r\n sequence
*/
int	joinParse(const std::string &buffer)
{
	std::string buff = buffer.substr(2, buffer.size() - 4);

	if (buffer.empty() == true || buffer[0] != ' ' || buffer[1] != '#' || (buffer.size() - 2) < 3)
		return (-1);
	int spaceCount = 0;
	for (size_t i = 0; i < buff.size(); i++)
	{
		if (!isalnum(buff[i]))
		{
			if (buff[i] == ' ')
				spaceCount++;
			if (spaceCount > 1 || buff[i] != ' ')
				return (-1);
		}
	}
	return (0);
}

int	Commands::joinLeaveAll(User& user, std::vector<Channel> &channelList) const
{
	for (std::vector<Channel>::iterator it = channelList.begin(); it != channelList.end(); it++)
	{
		std::vector<User*> members = it->getChanMembers();
		for (std::vector<User*>::iterator subiter = members.begin(); subiter != members.end(); subiter++)
		{
			if (user.nickname == (*subiter)->nickname)
			{
				std::string res(":");
				res += user.nickname + "!" + user.username + "@localhost PART " + it->getChanName() + " :Leaving\r\n";
				send((*subiter)->socket, res.c_str(), res.size(), 0);
				if ((*subiter)->nickname == user.nickname)
					it->removeMember(*subiter);
				for(std::vector<User *>::iterator opList = it->flags.operatorList.begin(); opList != it->flags.operatorList.end(); opList++)
				{
					if ((*opList)->nickname == user.nickname)
					{
						it->flags.operatorList.erase(opList);
						break ;
					}
				}
				break ;
			}
		}
	}
	return (0);
}

int	Commands::join(User& user, const std::string buffer, std::vector<Channel> &channelList) const
{
	std::string buff = buffer.substr(1, buffer.size() - 3);	//removing the space and \r\n to leave only '#channelname [params]'
	bool	channelExists = false;
	Channel *chan;
	Channel newChan;

	if (buff[0] == '0' && buff.size() == 1)
		return (joinLeaveAll(user, channelList));
	if (joinParse(buffer) < 0)
	{
		std::string BADCHANMASK(S_ERR_BADCHANMASK);
		BADCHANMASK += " " + buffer.substr(1, buffer.size() - 3) + " :Bad Channel Mask";
		return (sendNumericReply(user, BADCHANMASK));
	}
	for (std::vector<Channel>::iterator it = channelList.begin(); it != channelList.end(); it++)
	{
		if (buff == it->getChanName() || buff.substr(0, buff.find(' ', 0)) == it->getChanName())
		{
			channelExists = true;
			Channel &temp = *it;
			chan = &temp;
			break ;
		}
	}
	if (channelExists == true)
	{
		if (chan->flags.userLimit.first == true && chan->getChanMembers().size() == chan->flags.userLimit.second)
		{
			std::string CHANNELFULL(S_ERR_CHANNELISFULL);
			CHANNELFULL += " " + user.nickname + " " + chan->getChanName() + " :Cannot join channel (+l)";
			return (sendNumericReply(user, CHANNELFULL));
		}
		/*
			When all users leave a channel, it's not actually deleted from the server's memory.
			Therefore, if someone "rejoins" the channel that everyone left, that user should gain operator privileges.
		*/
		if (chan->getChanMembers().size() == 0)
			chan->flags.operatorList.push_back(&user);
		size_t posTry = buff.find(' ');
		std::string passTry;
		if (posTry != std::string::npos)
			passTry = buff.substr(posTry + 1);
		else
			passTry = buff;
		if (chan->flags.pswdIsSet.first == true && passTry != chan->flags.pswdIsSet.second)
		{
			std::string BADKEY(S_ERR_BADCHANNELKEY);
			BADKEY += " " + user.nickname + " " + chan->getChanName() + " :Cannot join channel (+k)";
			return (sendNumericReply(user, BADKEY));
		}
		std::vector<User*> members = chan->getChanMembers();
		for (std::vector<User*>::const_iterator it = members.begin(); it != members.end(); it++)
		{
			if (user.nickname == (*it)->nickname)
				return (0);
		}
		//EDIT STARTS HERE
		if (chan->flags.inviteOnly == true)
		{
			std::vector<User *> &invitedUsers = chan->flags.invitedUsers;
			bool invited = false;
			for (std::vector<User *>::iterator it = invitedUsers.begin(); it != invitedUsers.end(); it++)
			{
				if ((*it)->nickname == user.nickname)
				{
					invited = true;
					invitedUsers.erase(it);
					break ;
				}
			}
			if (invited == false)
			{
				std::string INVITEONLY(S_ERR_INVITEONLYCHAN);
				INVITEONLY += " " + user.nickname + " " + chan->getChanName() + " :Cannot join channel (+i)";
				return (sendNumericReply(user, INVITEONLY));
			}
		}
		//if there are other users, send JOIN to them
		if (chan->getChanMembers().size() > 0)
		{
			std::string joinRelayMessage(':' + user.nickname + '!' + user.username + '@' + "localhost " + "JOIN " + chan->getChanName() + "\r\n");
			for (std::vector<User*>::const_iterator subIter = members.begin(); subIter != members.end(); subIter++)
				send((*subIter)->socket, joinRelayMessage.c_str(), joinRelayMessage.size(), 0);
		}
		//join the channel
		chan->addMember(&user);
	}
	else
	{
		chan = &newChan;
		chan->setChanName(buff);
		chan->addMember(&user);
		chan->flags.operatorList.push_back(&user);
		channelList.push_back(*chan);
	}
	std::string joinReply(":" + user.nickname + "!" + user.username + "@" + "localhost" + " JOIN " + buff + "\r\n");
	send(user.socket, joinReply.c_str(), joinReply.size(), 0);
	if (chan->getChanTopic().empty() == false)
	{
		std::string RPLTOPIC(S_RPL_TOPIC);

		RPLTOPIC += ' ' + user.nickname + ' ' + chan->getChanName() + " :" + chan->getChanTopic();
		sendNumericReply(user, RPLTOPIC);
	}
	/*<client> <symbol> <channel> :[prefix]<nick>{ [prefix]<nick>}*/
	std::string NAMREPLY(S_RPL_NAMREPLY);

	NAMREPLY += " " + user.nickname + " = ";
	NAMREPLY += chan->getChanName();
	NAMREPLY += " :";

	std::vector<User*> members = chan->getChanMembers();
	for (std::vector<User*>::const_iterator it = members.begin(); it != members.end(); it++)
	{
		std::string name(NAMREPLY);
		for (std::vector<User *>::const_iterator subiter = chan->flags.operatorList.begin(); subiter != chan->flags.operatorList.end(); subiter++)
		{
			if ((*it)->nickname == (*subiter)->nickname)
				name += '@';
		}
		name += (*it)->nickname;
		if (it + 1 != members.end())
			name += ' ';
		sendNumericReply(user, name);
	}
	std::string ENDOFNAMES(S_RPL_ENDOFNAMES);

	ENDOFNAMES += ' ' + user.nickname + ' ' + chan->getChanName() + " :End of /NAMES list";
	sendNumericReply(user, ENDOFNAMES);
	return (0);
}

/*<channel> [<reason>]*/
int Commands::part(User& user, const std::string &buffer, std::vector<Channel> &channelList) const
{
	if (buffer.size() < 3 || buffer[0] != ' ') // || buffer[1] != '#'
		return (-1);

	std::string buff = buffer.substr(1, buffer.size() - 3);

	int spaceCount = 0;
	for (size_t i = 0; i < buffer.size(); i++)
	{
		if (buffer[i] == ' ' && buffer[i + 1] != ' ')
			spaceCount++;
	}
	if (spaceCount < 2)
	{
		std::string NEEDMORE(S_ERR_NEEDMOREPARAMS);
		NEEDMORE += " " + user.nickname + " PART :Not enough parameters";
		return (sendNumericReply(user, NEEDMORE));
	}
	if (channelList.empty())
	{
		std::string NOSUCHCHAN(S_ERR_NOSUCHCHANNEL);
		size_t hashPos = buff.find('#');
		NOSUCHCHAN += " " + user.nickname + " " + buff.substr(hashPos, buff.find(' ', buff.find('#'))) + " :No such channel";
		return (sendNumericReply(user, NOSUCHCHAN));
	}

	Channel *chan;
	for (std::vector<Channel>::iterator it = channelList.begin(); it != channelList.end(); it++)
	{
		bool err = false;
		size_t hashPos = buff.find('#');
		std::string chanName(buff.substr(hashPos, buff.find(' ', 0)));

		if (hashPos == std::string::npos)
			err = true;
		if (it->getChanName() == chanName)
		{
			Channel& temp = *it;
			chan = &temp;
			break ;
		}
		if (err == true || it + 1 == channelList.end())
		{
			std::string NOSUCHCHAN(S_ERR_NOSUCHCHANNEL);
			NOSUCHCHAN += " " + user.nickname + " " + buff.substr(hashPos, buff.find(' ', buff.find('#'))) + " :No such channel";
			return (sendNumericReply(user, NOSUCHCHAN));
		}
	}

	std::vector<User*> usrList = chan->getChanMembers();
	for (std::vector<User*>::iterator it = usrList.begin(); it != usrList.end(); it++)
	{
		if ((*it)->nickname == user.nickname)
			break;
		else if (it + 1 == usrList.end())
		{
			std::string NOTONCHAN(S_ERR_NOTONCHANNEL);
			NOTONCHAN += " " + user.nickname + " " + chan->getChanName() + " :You're not on that channel";
			return (sendNumericReply(user, NOTONCHAN));
		}
	}

	std::string res(":");
	res += user.nickname + "!" + user.username + "@localhost PART " + chan->getChanName();

	std::string reason;
	size_t spacePos;
	spacePos = buff.find_last_of(' ');
	reason = buff.substr(spacePos, buff.size());
	if (reason != chan->getChanName())
		res += reason + "\r\n";
	for(std::vector<User*>::iterator it = usrList.begin(); it != usrList.end(); it++)
	{
		send((*it)->socket, res.c_str(), res.size(), 0);
		if ((*it)->nickname == user.nickname)
			chan->removeMember(*it);
	}
	for(std::vector<User *>::iterator it = chan->flags.operatorList.begin(); it != chan->flags.operatorList.end(); it++)
	{
		if ((*it)->nickname == user.nickname)
		{
			chan->flags.operatorList.erase(it);
			break ;
		}
	}
	//If everyone leaves, clear all flags
	if (chan->getChanMembers().size() == 0)
	{
		chan->flags.invitedUsers.clear();
		chan->flags.inviteOnly = false;
		chan->flags.operatorList.clear();
		chan->flags.pswdIsSet.first = false;
		chan->flags.pswdIsSet.second.clear();
		chan->flags.topicOpOnly = false;
		chan->flags.userLimit.first = false;
		chan->flags.userLimit.second = 16;
	}
	return (0);
}

int Commands::quit(User& user, const std::string &buffer, std::vector<User*> &userList, std::vector<struct pollfd> &pfdsArr)
{
	if ((buffer[0] != ' ' && buffer[0] != '\0') && buffer.find(':') == std::string::npos)
		return (-1);

	std::string quitMsg(':' + user.nickname + '!' + user.username + "@localhost QUIT" + buffer);
	if (buffer[buffer.size() - 2] != '\r' && buffer[buffer.size() - 1] != '\n')
		quitMsg += "\r\n";
	for (std::vector<User*>::iterator it = userList.begin(); it != userList.end(); it++)
		send((*it)->socket, quitMsg.c_str(), quitMsg.size(), 0);
	for (std::vector<struct pollfd>::iterator it = pfdsArr.begin(); it != pfdsArr.end(); it++)
	{
		if (it->fd == user.socket)
		{
			pfdsArr.erase(it);
			break ;
		}
	}
	for (std::vector<User*>::iterator it = userList.begin(); it != userList.end(); it++)
	{
		if ((*it)->nickname == user.nickname)
		{
			delete *it;
			userList.erase(it);
			break ;
		}
	}
	return (0);
}

int	Commands::privmsgUser(User& user, const std::string &buffer, const std::string& target, const std::vector<User*> &userList) const
{
	size_t messagePos = buffer.find(":", 0);
	if (messagePos == std::string::npos)
		return (-1);
	const std::string message(buffer.substr(messagePos));

	for (std::vector<User*>::const_iterator it = userList.begin(); it != userList.end(); it++)
	{
		if (it + 1 == userList.end() && target != (*it)->nickname)
		{
			std::string NOSUCHNICK(S_ERR_NOSUCHNICK + ' ' + target + ' ' + user.nickname + " :No such nick/channel");
			return (sendNumericReply(user, NOSUCHNICK));
		}
		if (target == (*it)->nickname && target != user.nickname)
		{
			std::string fullMsg(':' + user.nickname + '!' + user.username + "@localhost " + "PRIVMSG" + buffer);
			return (send((*it)->socket, fullMsg.c_str(), fullMsg.size(), 0));
		}
	}
	return (0);
}

int	Commands::privmsgChannel(User& user, const std::string &buffer, const std::string &target, const std::vector<Channel> &channelList) const
{
	for (std::vector<Channel>::const_iterator it = channelList.begin(); it != channelList.end(); it++)
	{
		if (target == it->getChanName())
		{
			std::vector<User*> chanMembers = it->getChanMembers();
			//Full client identifier + PRIVMSG + #channel + :message
			std::string fullMsg(':' + user.nickname + '!' + user.username + "@localhost " + "PRIVMSG" + buffer);
			for (std::vector<User*>::iterator subiter = chanMembers.begin(); subiter != chanMembers.end(); subiter++)
			{
				if ((*subiter)->nickname == user.nickname)
					break ;
				if (subiter + 1 == chanMembers.end())
					return (-1);
			}
			signal(SIGPIPE, SIG_IGN);
			for (std::vector<User*>::iterator subiter = chanMembers.begin(); subiter != chanMembers.end(); subiter++)
			{
				if ((*subiter)->nickname != user.nickname)
					send((*subiter)->socket, fullMsg.c_str(), fullMsg.size(), 0);
			}
			return (0);
		}
	}
	return (0);
}


/*:<source> PRIVMSG <target> :<message>*/
int		Commands::privmsg(User& user, const std::string &buffer, const std::vector<Channel> &channelList, const std::vector<User*> &userList) const
{
	//check if target is a user, or if it's a channel
		//if neither, error NOSUCHNICK i think
		//if user, send it
		//if channel, find out who's in it, and send to all of them

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
	if (target[0] == '#')
		return (privmsgChannel(user, buffer, target, channelList));
	else
		return (privmsgUser(user, buffer, target, userList));
	return (0);
}
