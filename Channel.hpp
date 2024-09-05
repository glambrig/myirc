#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <utility>
#include <ctime>
#include "User.hpp"

typedef struct ModeFlags
{
	bool							inviteOnly;		//+-i
	std::vector<User *>				invitedUsers;
	bool							topicOpOnly;	//+-t
	std::pair<bool, size_t>			userLimit;		//+-l
	std::pair<bool, std::string>	pswdIsSet;		//+-k
	std::vector<User *>				operatorList;	//+-o
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
	LastTopic			_lastTopic;
public:
	Channel();
	Channel(const Channel& copy);
	Channel& operator=(const Channel& rhs);
	~Channel();

	ModeFlags			flags;
	std::vector<User*>	_chanMembers;	//terrible, but way easier than turning it into User* everywhere

	void				setChanName(const std::string name);
	std::string			getChanName() const;

	std::string			getChanTopic() const;
	void				setChanTopic(const std::string s);
	LastTopic			getLastTopic();
	void				setLastTopic(const std::string s);

	std::vector<User*>	getChanMembers() const;
	void				addMember(User* user);
	void				removeMember(User* user);
};
