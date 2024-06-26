#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <utility>
#include "User.hpp"

typedef struct ModeFlags
{
	// bool							opPrivilege; //not sure what this means yet
	bool							inviteOnly;
	bool							topicOpOnly;
	std::pair<bool, std::string>	pswdIsSet;
	std::pair<bool, unsigned int>	usrLimit;
}	ModeFlags;

class Channel
{
private:
	std::string			_name;
	std::vector<User>	_chanMembers;
public:
	Channel();
	Channel(const Channel& copy);
	Channel& operator=(const Channel& rhs);
	~Channel();

	ModeFlags			flags;

	void				setChanName(const std::string name);
	std::string			getChanName() const;
	std::vector<User>	getChanMembers() const;
	void				addMember(User &user);
	void				removeMember(User &user);
};