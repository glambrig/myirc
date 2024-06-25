#include "Channel.hpp"

Channel::Channel()
{

}

Channel::Channel(const Channel& copy)
{
	if (this != copy)
	{
		*this = copy;
	}
}

Channel& Channel::operator=(const Channel& rhs)
{
	if (*this != rhs)
	{
		*this = rhs;
	}
}

Channel::~Channel()
{

}

std::vector<User>	Channel::getChanMembers() const
{
	return (this->_chanMembers);
}

void	Channel::addMember(User &user)
{
	_chanMembers.push_back(user);
}

void	Channel::removeMember(User &user)
{
	for (std::vector<User>::iterator it = _chanMembers.begin(); it != _chanMembers.end(); it++)
	{
		if (*it.nickname == user.nickname)
		{
			_chanMembers.erase(it);
			return ;
		}
	}
}
