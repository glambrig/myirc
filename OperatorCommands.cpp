#include "Commands.hpp"

/*Commands specific to channel operators*/

char	*numToStr(unsigned long long num)
{
	int		len = 0;
	unsigned long long n = num;
	char	*res;

	while (num > 0)
	{
		len++;
		num /= 10;
	}
	res = new char[len + 1];
	for (int i = len - 1; i > -1; i--)
	{
		res[i] = n % 10 + '0';
		n /= 10;
	}
	res[len] = '\0';
	return (res);
}

/*Syntax: TOPIC #<channel> [:<topic>]*/
int	Commands::topic(const User& user, const std::string& buff, std::vector<Channel> &channelList) const
{
	Channel *chan;

	if (buff.size() > 1 && (buff[0] != ' ' || buff[1] != '#'))
		return (-1);
	for (std::vector<Channel>::iterator it = channelList.begin(); it != channelList.end(); it++)
	{
		std::string name(buff.substr(1, buff.find(":", 1) - 1));
		if (name[name.size() - 1] == ' ')
			name.erase(name.size() - 1, 1);
		if (name.find("\r\n") != std::string::npos)
			name = name.substr(0, name.find("\r\n"));
		// std::cout << "name: " << '\'' << name << '\'' << std::endl;
		if (it->getChanName() == name)
		{
			Channel& temp = *it;
			chan = &temp;
			break ;
		}
	}
	if (chan == NULL || chan->getChanName().empty())
		return (-1);
	bool userInChannel = false;
	std::vector<User*> members = chan->getChanMembers();
	for (std::vector<User*>::iterator it = members.begin(); it != members.end(); it++)
	{
		if (user.nickname == (*it)->nickname)
		{
			userInChannel = true;
			break ;
		}
	}
	if (userInChannel == false)
	{
		std::string NOTONCHANNEL(S_ERR_NOTONCHANNEL);
		NOTONCHANNEL += user.nickname + ' ' + chan->getChanName() + " :You're not on that channel";
		return (sendNumericReply(user, NOTONCHANNEL));
	}
	//if topic is not given, reply with topic/notopic
	if (buff.empty() || buff.find(":") == std::string::npos)
	{
		if (chan->getChanTopic().empty())
		{
			std::string NOTOPIC(S_RPL_NOTOPIC);
			NOTOPIC += " TOPIC " + chan->getChanName() + " :No topic is set";
			return (sendNumericReply(user, NOTOPIC));
		}
		else
		{
			std::string TOPIC(S_RPL_TOPIC);
			TOPIC += " TOPIC " + chan->getChanName() + " :" + chan->getChanTopic();
			std::string TOPICWHOTIME(S_RPL_TOPICWHOTIME);
			char *currentTime = numToStr(static_cast<unsigned long long>(chan->getLastTopic().lastTopicSetAt));
			TOPICWHOTIME += " TOPIC " + user.nickname + ' ' + chan->getChanName() + ' ' + chan->getLastTopic().lastTopicSetBy + ' ' + currentTime;
			sendNumericReply(user, TOPIC);
			// std::cout << "time: " << chan->getLastTopic().lastTopicSetAt << std::endl;
			// std::cout << "TOPICWHOTIME:" << '\'' << TOPICWHOTIME << '\'' << std::endl;
			delete[] currentTime;
			return (sendNumericReply(user, TOPICWHOTIME));
		}
	}
	else
	{
		bool userIsOp = false;
		std::vector<User *> opList = chan->flags.operatorList;
		for (std::vector<User *>::const_iterator it = opList.begin(); it != opList.end(); it++)
		{
			if ((*it)->nickname == user.nickname)
			{
				userIsOp = true;
				break ;
			}
		}
		if (chan->flags.topicOpOnly == true && userIsOp == false)
		{
			std::string CHANOPRIVSNEEDED(S_ERR_CHANOPRIVSNEEDED);
			CHANOPRIVSNEEDED += ' ' + user.nickname + ' ' + chan->getChanName() + " :You're not a channel operator";
			return (sendNumericReply(user, CHANOPRIVSNEEDED));
		}
		else
		{
			//user can change topic
			std::string newTopic(buff.substr(buff.find(":") + 1));
			newTopic.erase(newTopic.size() - 2, 2);
			chan->setChanTopic(newTopic);
			chan->setLastTopic(user.nickname);
			members.clear();
			members = chan->getChanMembers();
			for (std::vector<User*>::const_iterator it = members.begin(); it != members.end(); it++)
			{
				std::string TOPIC(':' + user.nickname + " TOPIC " + chan->getChanName() + ' ' + newTopic + "\r\n");
				send((*it)->socket, TOPIC.c_str(), TOPIC.size(), 0);
			}
		}
	}
	return (0);
}

std::string	emptyModeCommand(const User& user, const Channel *chan)
{
	std::string MODEIS(S_RPL_CHANNELMODEIS);
	bool usrLimit = false;
	bool pswdSet = false;

	MODEIS += ' ' + user.nickname + ' ' + chan->getChanName() + " +";
	if (chan->flags.inviteOnly == true)
		MODEIS += "i";
	if (chan->flags.pswdIsSet.first == true)
	{
		MODEIS += "k";
		pswdSet = true;
	}
	if (chan->flags.topicOpOnly == true)
		MODEIS += "t";
	if (chan->flags.userLimit.first == true)
	{
		MODEIS += "l";
		usrLimit = true;
	}
	if (pswdSet)
		MODEIS += chan->flags.pswdIsSet.second + ' ';
	if (usrLimit)
		MODEIS += chan->flags.userLimit.second;
	return (MODEIS);
}

void removeModesDupes(std::string &modes)
{
	std::string asdf;

	for (size_t i = 0; modes[i] && modes[i] != ' ';)
	{
		if (asdf.find(modes[i], 0) == std::string::npos)
		{
			asdf += modes[i];
			i++;
		}
		else
			modes.erase(modes.begin() + i);
	}
}

/*Syntax: MODE <channel> <modes> [mode params]*/
int	Commands::mode(const User& user, const std::string buffer, std::vector<Channel> &channelList) const
{
	std::string buff(buffer.substr(0, buffer.size() - 2));
	if (buff.size() < 3 || buff[0] != ' ' || buff[1] != '#')
		return (-1);

	Channel *chan;

	for (std::vector<Channel>::iterator it = channelList.begin(); it != channelList.end(); it++)
	{
		size_t spacePos = buff.find(' ', 1);
		std::string chanName;
		if (spacePos != std::string::npos)
			chanName = buff.substr(1, spacePos - 1);
		else
			chanName = buff.substr(1, spacePos);

		if (it->getChanName() == chanName)
		{
			Channel& temp = *it;
			chan = &temp;
			break ;
		}
		if (it + 1 == channelList.end())
			return (-1);
	}
	std::string modes(buff.substr(buff.find(' ', 1) + 1));	//everything after "MODE #channel "

	removeModesDupes(modes);
	std::string shortenedBuff = modes;
	if (modes.empty() || modes == buff)	//' ' wasn't found in buff
		return (sendNumericReply(user, emptyModeCommand(user, chan)));
	//check if user is op
	std::vector<User *> &opList = chan->flags.operatorList;
	for (std::vector<User *>::const_iterator it = opList.begin(); it != opList.end(); it++)
	{
		if (user.nickname == (*it)->nickname)
			break ;
		if (it + 1 == opList.end())
			return (-1);
	}

	bool plus = false;
	for (size_t i = 0; modes[i];)// && modes[i] != ' '
	{
		//check if it exists (it's a real mode)
		if (modes[i] != 'i' && modes[i] != 't' && modes[i] != 'k' && modes[i] != 'l' && modes[i] != 'o' && modes[i] != '+' && modes[i] != '-' && modes[i] != ' ')
		{
			//needs the throw ERR_UNKNOWNMODE (472) instead if flag targets a channel(if flag == [i, t, k, l])
			std::string UNKNOWNFLAG(S_ERR_UMODEUNKNOWNFLAG);

			UNKNOWNFLAG += ' ' + user.nickname + " :Unknown MODE flag";
			return (sendNumericReply(user, UNKNOWNFLAG));
		}
		//check whether we're adding flags or removing them
		if (modes[i] == '+' || modes[i] == '-')
		{
			if (modes[i + 1] == '+' || modes[i + 1] == '-')	//check for consecutive +/-
				return (-1);
			if (modes[i] == '+')
				plus = true;
			else if (modes[i] == '-')
				plus = false;
			for (std::string::iterator it = modes.begin();;)
			{
				size_t k = 0;
				if (i == 0)
				{
					modes.erase(it);
					break ;
				}
				while (k++ < i - 1)
					it++;
				modes.erase(it);
				break ;
			}
		}

		std::string params(modes);	//Any parameters that the modes might have (e.g. +-o <nickname>)
		params = params.substr(params.find(' ') + 1);
		switch (modes[i])
		{
			case 'i':
			{
				if (plus)
					chan->flags.inviteOnly = true;
				else
					chan->flags.inviteOnly = false;
				break ;
			}
			case 't':
			{
				if (plus)
					chan->flags.topicOpOnly = true;
				else
					chan->flags.topicOpOnly = false;
				break ;
			}
			case 'l':
			{
				if (plus)
				{
					std::string temp(params.substr(0, params.find(" \0")));
					if (temp.find_first_not_of("0123456789") == std::string::npos)
					{
						chan->flags.userLimit.first = true;
						chan->flags.userLimit.second = atoi(temp.c_str());
					}
				}
				else
					chan->flags.userLimit.first = false;
				break ;
			}
			case 'k':
			{
				if (plus)
				{
					chan->flags.pswdIsSet.first = true;
					size_t pos = params.find(' ');
					if (pos != std::string::npos)
						chan->flags.pswdIsSet.second = params.substr(0, pos);
					else if (params == modes)
						return (-1);
					else
						chan->flags.pswdIsSet.second = params;
				}
				else
					chan->flags.pswdIsSet.first = false;
				break ;
			}
			case 'o':
			{
				size_t pos = params.find(' ');
				std::string asdf;
				if (pos != std::string::npos)
					asdf = params.substr(0, pos);
				else if (params == modes)	//No parameters provided when there should be
						return (-1);
				else
					asdf = params;

				// std::cout << "operatorList.size() = " << chan->flags.operatorList.size() << std::endl;

				std::vector<User*> members = chan->getChanMembers();
				for (std::vector<User*>::iterator it = members.begin(); it !=  members.end(); it++)
				{
					if ((*it)->nickname == asdf)
					{
						if (plus)
						{
							bool targetIsOp = false;
							for (std::vector<User *>::iterator it = opList.begin(); it != opList.end(); it++)
							{
								if ((*it)->nickname == asdf)
								{
									targetIsOp = true;
									break ;
								}
							}
							if (targetIsOp == true)	//If target is already OP, don't push onto operatorList
								break ;
							chan->flags.operatorList.push_back(*it);
						}
						else
						{
							for (std::vector<User *>::iterator subiter = opList.begin(); subiter != opList.end(); subiter++)
							{
								if ((*subiter)->nickname == asdf)
								{
									opList.erase(subiter);
									// std::cout << "size after:" << opList.size() << std::endl;
									break ;
								}
							}
						}
						break ;
					}
					if (it + 1 == members.end())
					{
						std::string NOSUCHNICK(S_ERR_NOSUCHNICK);
						NOSUCHNICK += " " + user.nickname + " " + (*it)->nickname + " :No such nick";
						return (sendNumericReply(user, NOSUCHNICK));
					}
				}
				break ;
			}
			default:
			{
				if (modes[i] == ' ')
					break ;
				i++;
				continue ;
			}
		}
		//If we're finished (there are no more flags to handle), get out of the loop
		if (modes[i] == ' ')
			break ;
		//erase the resolved flag from the string
		for (std::string::iterator it = modes.begin(); it != modes.end(); it++)
		{
			if (*it == modes[i] && modes[i] != '+' && modes[i] != '-')
			{
				modes.erase(it);
				break ;
			}
		}
		for (std::string::iterator it = modes.begin(); it != modes.end() && plus; it++)
		{
			if (modes.find(' ') != std::string::npos)
			{
				while (*it && *it != ' ')
					it++;
				modes.erase(it);
				while (*it && *it != ' ')
					modes.erase(it);
				break ;
			}
		}
	}
	std::string reply(":" + user.nickname + " MODE " + chan->getChanName() + " " + shortenedBuff + "\r\n");
	std::vector<User*> members = chan->getChanMembers();
	for (std::vector<User*>::const_iterator it = members.begin(); it != members.end(); it++)
		send((*it)->socket, reply.c_str(), reply.size(), 0);
	return (0);
}

bool checkOpPrivs(const User& user, const Channel *chan)
{
	bool userIsOp = false;
	std::vector<User *> opList = chan->flags.operatorList;
	for (std::vector<User *>::const_iterator it = opList.begin(); it != opList.end(); it++)
	{
		if ((*it)->nickname == user.nickname)
		{
			userIsOp = true;
			break ;
		}
	}
	if (chan->flags.topicOpOnly == true && userIsOp == false)
	{
		std::string CHANOPRIVSNEEDED(S_ERR_CHANOPRIVSNEEDED);
		CHANOPRIVSNEEDED += ' ' + user.nickname + ' ' + chan->getChanName() + " :You're not a channel operator";
		Commands::sendNumericReply(user, CHANOPRIVSNEEDED);
	}
	return (userIsOp);
}
/*Syntax: KICK <channel> <user> [<comment>]*/
int	Commands::kick(const User& user, std::vector<Channel>& channelList, const std::string buffer) const
{
	if (buffer.size() - 2 < 5 || buffer[0] != ' ' || buffer.find('#') == std::string::npos || buffer.find("\r\n") == std::string::npos)
		return (-1);

	std::string buff = buffer.substr(0, buffer.size() - 2);

	int spaceCount = 0;
	for (size_t i = 0; i < buff.size(); i++)
	{
		if (buff[i] == ' ' && buff[i - 1] != ' ')
			spaceCount++;
	}
	if (spaceCount < 2)
	{
		std::string NEEDMORE(S_ERR_NEEDMOREPARAMS);
		NEEDMORE += " " + user.nickname + " KICK :Not enough parameters";
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
		std::string chanName(buff.substr(hashPos, buff.find(' ', buff.find('#')) - 1));

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

	std::vector<User*> memberList = chan->_chanMembers;
	// std::vector<User> memberList = chan->getChanMembers();
	std::string temp(buff.substr(1, buff.size() - 1));
	std::string targetUserStr(temp.substr(temp.find(' ', 0) + 1));
	//if there's a comment/reason afterwards, trim targetUserStr
	if (targetUserStr.find(' ', 0) != std::string::npos)
		targetUserStr = targetUserStr.substr(0, targetUserStr.find(' ', 0));
	//user can't kick themselves
	if (targetUserStr == user.nickname)
		return (-1);
	if (memberList.empty() == true)
	{
		std::string NOSUCHNICK(S_ERR_NOSUCHNICK);
		NOSUCHNICK += " " + user.nickname + " " + targetUserStr + " :No such nick";
		return (sendNumericReply(user, NOSUCHNICK));
	}
	for (std::vector<User*>::iterator it = memberList.begin(); it != memberList.end(); it++)
	{
		if (targetUserStr == (*it)->nickname)
		{
			//if user isn't on chan, error
			for (std::vector<User*>::iterator subiter = memberList.begin(); subiter != memberList.end(); subiter++)
			{
				if (user.nickname == (*subiter)->nickname)
					break ;
				else if (subiter + 1 == memberList.end())
				{
					std::string NOTONCHAN(S_ERR_NOTONCHANNEL);
					NOTONCHAN += " " + user.nickname + " " + chan->getChanName() + " :You're not on that channel";
					return (sendNumericReply(user, NOTONCHAN));
				}
			}
			if (checkOpPrivs(user, chan) == false)
				return (-1);
			for (std::vector<User*>::iterator replyiter = memberList.begin(); replyiter != memberList.end(); replyiter++)
			{
				std::string reply(":");
				reply += user.nickname + "!" + user.username + "@localhost KICK" + buff + "\r\n";
				send((*replyiter)->socket, reply.c_str(), reply.size(), 0);
			}
			// chan->_chanMembers.erase(it);
			chan->removeMember(*it);
			return (0);
		}
		else if (it + 1 == memberList.end())
		{
			std::string NOSUCHNICK(S_ERR_NOSUCHNICK);
			NOSUCHNICK += " " + user.nickname + " " + targetUserStr + " :No such nick";
			return (sendNumericReply(user, NOSUCHNICK));
		}
	}
	return (-1);
}

int inviteParsing(const std::string buffer, std::string &targetUser)
{
	if (buffer.size() - 2 < 5 || buffer[0] != ' ' || buffer.find('#') == std::string::npos || buffer.find("\r\n") == std::string::npos)
		return (-1);

	std::string buff = buffer.substr(0, buffer.size() - 2);

	int spaceCount = 0;
	for (size_t i = 0; i < buff.size(); i++)
	{
		if (buff[i] == ' ' && buff[i - 1] != ' ')
			spaceCount++;
	}
	if (spaceCount < 2 || buff.find('#', 1) == std::string::npos)
		return (-1);
	targetUser = buff.substr(1, buff.find(' ', 1) - 1);
	return (0);
}

/*Syntax: INVITE <nickname> <channel>*/
int	Commands::invite(const User& user, const std::string buffer, std::vector<Channel> &channelList, std::vector<User*> &userList) const
{
	std::string targetUser;
	Channel *chan;

	if (inviteParsing(buffer, targetUser) < 0)
		return (-1);

	std::string buff = buffer.substr(0, buffer.size() - 2);

	//target channel should exist
	for (std::vector<Channel>::iterator it = channelList.begin(); it != channelList.end(); it++)
	{
		size_t pos = buff.find('#', 1);
		if (pos == std::string::npos)
		{
			std::string NOSUCHCHAN(S_ERR_NOSUCHCHANNEL);

			NOSUCHCHAN += " " + user.nickname + " " + chan->getChanName() + " :No such channel";
			return (sendNumericReply(user, NOSUCHCHAN));
		}
		std::string chanName(buff.substr(pos));
		if (it->getChanName() == chanName)
		{
			Channel &temp = *it;
			chan = &temp;
			break ;
		}
		else if (it + 1 == channelList.end())
		{
			std::string NOSUCHCHANNEL(S_ERR_NOSUCHCHANNEL);

			NOSUCHCHANNEL += " " + user.nickname + " " + chanName + " :No such channel";
			return (sendNumericReply(user, NOSUCHCHANNEL));
		}
	}
	//Only members of the channel are allowed to invite other users
	std::vector<User*> members = chan->getChanMembers();
	for (std::vector<User*>::iterator it = members.begin(); it != members.end(); it++)
	{
		if (user.nickname == (*it)->nickname)
			break ;
		else if (it + 1 == members.end())
		{
			std::string NOTONCHAN(S_ERR_NOTONCHANNEL);

			NOTONCHAN += " " + user.nickname + " " + chan->getChanName() + " :You're not on that channel";
			return (sendNumericReply(user, NOTONCHAN));
		}
	}
	if (chan->flags.inviteOnly == true)
	{
		std::vector<User *> opList = chan->flags.operatorList;
		for (std::vector<User *>::iterator it = opList.begin(); it != opList.end(); it++)
		{
			if (user.nickname == (*it)->nickname)
				break ;
			else if (it + 1 == opList.end())
			{
				std::string NEEDOPRIVS(S_ERR_CHANOPRIVSNEEDED);

				NEEDOPRIVS += " " + user.nickname + " " + chan->getChanName() + " :You're not channel operator";
				return (sendNumericReply(user, NEEDOPRIVS));
			}
		}
	}
	//If the user is already on the target channel, the server MUST reject the command with the ERR_USERONCHANNEL numeric.
	for (std::vector<User*>::const_iterator it = members.begin(); it != members.end(); it++)
	{
		if ((*it)->nickname == targetUser)
		{
			//<client> <nick> <channel> :is already on channel
			std::string USERONCHANNEL(S_ERR_USERONCHANNEL);

			USERONCHANNEL += " " + targetUser + " " + chan->getChanName() + " :is already on channel";
			return (sendNumericReply(user, USERONCHANNEL));
		}
	}
	std::string INVITING(S_RPL_INVITING);
	std::string inviteReply(":");
	User *invitedUser;

	INVITING += " " + user.nickname + " " + targetUser + " " + chan->getChanName();
	inviteReply += user.nickname + " INVITE " + targetUser + " " + chan->getChanName() + "\r\n";
	sendNumericReply(user, INVITING);
	for (std::vector<User*>::iterator it = userList.begin(); it != userList.end(); it++)
	{
		if ((*it)->nickname == targetUser)
		{
			User* temp = *it;
			invitedUser = temp;
			chan->flags.invitedUsers.push_back(invitedUser);
			return (send((*it)->socket, inviteReply.c_str(), inviteReply.size(), 0));
		}
		else if (it + 1 == userList.end())
		{
			std::string NOSUCHNICK(S_ERR_NOSUCHNICK);

			NOSUCHNICK += " " + user.nickname + " " + targetUser + " :No such nick";
			return (sendNumericReply(user, NOSUCHNICK));
		}
	}
	return (-1);
}
