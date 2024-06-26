#pragma once

#include <iostream>
#include <string>
#include <list>
#include <vector>
#include "User.hpp"
#include "Channel.hpp"

#define RPL_WELCOME 001
#define S_RPL_WELCOME "001"
#define RPL_YOURHOST 002
#define S_RPL_YOURHOST "002"
#define RPL_CREATED 003
#define S_RPL_CREATED "003"

//NICK codes
#define ERR_NONICKNAMEGIVEN 431
#define ERR_ERRONEUSNICKNAME 432
#define ERR_NICKNAMEINUSE 433
#define S_ERR_NONICKNAMEGIVEN "431"
#define S_ERR_ERRONEUSNICKNAME "432"
#define S_ERR_NICKNAMEINUSE "433"

//USER codes
#define ERR_NEEDMOREPARAMS 461
#define ERR_ALREADYREGISTERED 462
#define S_ERR_NEEDMOREPARAMS "461"
#define S_ERR_ALREADYREGISTERED "462"

//JOIN codes
#define ERR_BADCHANMASK 476
#define S_ERR_BADCHANMASK "476"

typedef struct Commands
{
	std::list<std::string>	cmdList;

	Commands();

	bool	hasRegistered;	//true if USER command has already been sent

	bool	isValidCommand(const std::string &s) const;
	int		sendNumericReply(const User &user, const std::string& err) const;

	int		nick(User& user, const std::string buff, std::vector<User> userList) const;
	int		parseUserBuff(const std::string &buff) const;
	int		user(User& user, const std::string buff) const;
	int		join(User& user, const std::string buff, std::vector<Channel> channelList) const;
}	Commands;