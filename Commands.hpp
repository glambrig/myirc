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
#define RPL_TOPIC 332
#define S_RPL_TOPIC "332"

//PRIVMSG codes
#define ERR_NORECIPIENT 411
#define S_ERR_NORECIPIENT "411"
#define ERR_NOSUCHNICK 401
#define S_ERR_NOSUCHNICK "401"

//TOPIC codes
#define RPL_NOTOPIC 331
#define S_RPL_NOTOPIC "331"
#define RPL_TOPIC 332
#define S_RPL_TOPIC "332"
#define ERR_CHANOPRIVSNEEDED 482
#define S_ERR_CHANOPRIVSNEEDED "482"
#define ERR_NOTONCHANNEL 442
#define S_ERR_NOTONCHANNEL "442"
#define RPL_TOPICWHOTIME 333
#define S_RPL_TOPICWHOTIME "333"

typedef struct Commands
{
	//If the nickname is invalid, the server will still run the USER command that was sent along with the first NICK command.
		//This is problematic because USER needs a nickname to return properly.
		//As such, we store the user command in here, and execute it after receiving the correct nick.
	std::string userCommand;
	std::list<std::string>	cmdList;

	Commands();

	bool	hasRegistered;	//true if USER command has already been sent

	bool	isValidCommand(const std::string &s) const;
	int		sendNumericReply(const User &user, const std::string& err) const;

	int		nick(User& user, const std::string buff, std::vector<User> userList);
	int		parseUserBuff(const std::string &buff) const;
	int		user(User& user, std::string buff);
	int		join(User& user, const std::string buff, std::vector<Channel> &channelList) const;
	int		privmsg(User& user, const std::string &buffer, const std::vector<Channel> channelList, const std::vector<User> userList) const;
	int		privmsgUser(User& user, const std::string &buffer, const std::string& target, const std::vector<User> &userList) const;
	int		privmsgChannel(User& user, const std::string &buffer, const std::string &target, const std::vector<Channel> channelList) const;

	/*Operator Commands*/
	int		topic(const User& user, const std::string& buff, std::vector<Channel> &channelList) const;
}	Commands;