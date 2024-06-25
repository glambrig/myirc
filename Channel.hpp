#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <User.hpp>

class Channel
{
private:
	std::vector<User>	_chanMembers;
public:
	Channel();
	Channel(const Channel& copy);
	Channel& operator=(const Channel& rhs);
	~Channel();

	std::vector<User>	getChanMembers() const;
	void						addMember(User &user);
	void						removeMember(User &user);
};