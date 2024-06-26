#include "Channel.hpp"

Channel::Channel()
{
	flags.inviteOnly = false;
	flags.topicOpOnly = false;
	flags.pswdIsSet.first = false;
	flags.usrLimit.first = false;
}

Channel::Channel(const Channel& copy)
{
	if (this != &copy)
	{
		*this = copy;
	}
}

Channel& Channel::operator=(const Channel& rhs)
{
	(void)rhs;
	return (*this);
}

Channel::~Channel()
{

}

std::string			Channel::getChanName() const
{
	return (this->_name);
}

void	Channel::setChanName(const std::string name)
{
	this->_name = name;
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
		if ((*it).nickname == user.nickname)
		{
			_chanMembers.erase(it);
			return ;
		}
	}
}
