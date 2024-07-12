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

	if (buff.size() > 1 && buff[0] != ' ' && buff[1] != '#')
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
	for (std::vector<User>::iterator it = chan->getChanMembers().begin(); it != chan->getChanMembers().end(); it++)
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
		for (std::vector<User>::const_iterator it = chan->flags.operatorList.begin(); it != chan->flags.operatorList.end(); it++)
		{
			if (it->nickname == user.nickname)
			{
				userIsOp = true;
				break ;
			}
		}
		if (chan->flags.topicOpOnly == true && userIsOp == false)
		{
			std::string CHANOPRIVSNEEDED(S_ERR_CHANOPRIVSNEEDED + ' ' + user.nickname + ' ' + chan->getChanName() + " :You're not a channel operator");
			return (sendNumericReply(user, CHANOPRIVSNEEDED));
		}
		else
		{
			//user can change topic
			std::string newTopic(buff.substr(buff.find(":") + 1));
			newTopic.erase(newTopic.size() - 2, 2);
			// std::cout << "newtopic: " << '\'' << newTopic << '\'' << std::endl;
			chan->setChanTopic(newTopic);
			chan->setLastTopic(user.nickname);
			for (std::vector<User>::const_iterator it = chan->getChanMembers().begin(); it != chan->getChanMembers().end(); it++)
			{
				std::string TOPIC(':' + user.nickname + " TOPIC " + chan->getChanName() + ' ' + newTopic + "\r\n");
				send(it->socket, TOPIC.c_str(), TOPIC.size(), 0);
			}
		}
	}
	return (0);
}

// int	Commands::kick()
// {

// }

// int	Commands::invite()
// {

// }

// int	Commands::mode()
// {

// }
