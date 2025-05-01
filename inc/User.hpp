#pragma once

#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>

typedef struct	User
{
	bool hasRegistered;
	bool enteredPass;
	std::string	username;
	std::string	nickname;
	int 		socket;

	User();

}	User;