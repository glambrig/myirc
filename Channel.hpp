#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <utility>
#include <ctime>
#include "User.hpp"

typedef struct ModeFlags
{
	bool							inviteOnly;
	bool							topicOpOnly;
	std::pair<bool, int>			userLimit;
	std::pair<bool, std::string>	pswdIsSet;
	std::vector<User>				operatorList;
}	ModeFlags;

typedef struct LastTopic
{
	std::string	lastTopicSetBy;		//nickname of the user who set the topic last
	std::time_t	lastTopicSetAt;		//time the last topic was set
}	LastTopic;

class Channel
{
private:
	std::string			_name;
	std::string			_topic;
	std::vector<User>	_chanMembers;
	LastTopic			_lastTopic;
public:
	Channel();
	Channel(const Channel& copy);
	Channel& operator=(const Channel& rhs);
	~Channel();

	ModeFlags			flags;

	void				setChanName(const std::string name);
	std::string			getChanName() const;

	std::string			getChanTopic() const;
	void				setChanTopic(const std::string s);
	LastTopic			getLastTopic();
	void				setLastTopic(const std::string s);

	std::vector<User>	getChanMembers() const;
	void				addMember(User &user);
	void				removeMember(User &user);
};