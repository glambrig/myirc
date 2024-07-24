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
	std::vector<User> members = chan->getChanMembers();
	for (std::vector<User>::iterator it = members.begin(); it != members.end(); it++)
	{
		if (user.nickname == it->nickname)
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
		std::vector<User> opList = chan->flags.operatorList;
		for (std::vector<User>::const_iterator it = opList.begin(); it != opList.end(); it++)
		{
			if (it->nickname == user.nickname)
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
			for (std::vector<User>::const_iterator it = members.begin(); it != members.end(); it++)
			{
				std::string TOPIC(':' + user.nickname + " TOPIC " + chan->getChanName() + ' ' + newTopic + "\r\n");
				send(it->socket, TOPIC.c_str(), TOPIC.size(), 0);
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
	
	if (modes.empty() || modes == buff)	//' ' wasn't found in buff
		return (sendNumericReply(user, emptyModeCommand(user, chan)));
	//check if user is op
	std::vector<User> opList = chan->flags.operatorList;
	for (std::vector<User>::const_iterator it = opList.begin(); it != opList.end(); it++)
	{
		if (user.nickname == it->nickname)
			break ;
		if (it + 1 == opList.end())
			return (-1);
	}

	bool plus = false;
	for (size_t i = 0; modes[i] && modes[i] != ' ';)
	{
		//check if it exists (it's a real mode)
		if (modes[i] != 'i' && modes[i] != 't' && modes[i] != 'k' && modes[i] != 'l' && modes[i] != 'o' && modes[i] != '+' && modes[i] != '-')
		{
			//needs the throw ERR_UNKNOWNMODE (472) instead if flag targets a channel(if flag == [i, t, k, l])
			std::string UNKNOWNFLAG(S_ERR_UMODEUNKNOWNFLAG);

			UNKNOWNFLAG += ' ' + user.nickname + " :Unknown MODE flag";
			return (sendNumericReply(user, UNKNOWNFLAG));
		}
		//check whether we're adding flags or removing them
		if (modes[i] == '+' || modes[i] == '-')
		{
			if (modes[i] + 1 == '+' || modes[i] + 1 == '-')	//check for consecutive +/-
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
		
		std::string params(modes);
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
						chan->flags.pswdIsSet.second = params.substr(0, pos - 1);
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
				else
					asdf = params;
				
				std::cout << chan->flags.operatorList.size() << std::endl;

				std::vector<User> members = chan->getChanMembers();
				for (std::vector<User>::iterator it = members.begin(); it !=  members.end(); it++)
				{
					if (it->nickname == asdf)
					{
						if (plus)
							chan->flags.operatorList.push_back(*it);
						else
						{
							std::vector<User> opList = chan->flags.operatorList;
							for (std::vector<User>::iterator subiter = opList.begin(); subiter != opList.end(); subiter++)
							{
								if (subiter->nickname == asdf)
								{
									opList.erase(subiter);
									std::cout << "after:" << opList.size() << std::endl;
									break ;
								}
							}
						}
						break ;
					}
					if (it + 1 == members.end())
					{
						std::string NOSUCHNICK(S_ERR_NOSUCHNICK);
						NOSUCHNICK += " " + user.nickname + " " + it->nickname + " :No such nick";
						return (sendNumericReply(user, NOSUCHNICK));
					}
				}
				break ;
			}
			default:
				i++;
				continue ;
		}
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
	std::string reply(":" + user.nickname + " MODE" + buffer);
	std::vector<User> members = chan->getChanMembers();
	for (std::vector<User>::const_iterator it = members.begin(); it != members.end(); it++)
		send(it->socket, reply.c_str(), reply.size(), 0);
	return (0);
}

// int	Commands::kick()
// {

// }

// int	Commands::invite()
// {

// }
