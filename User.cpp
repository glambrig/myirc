#include "User.hpp"
#include "Commands.hpp"

// void		User::setup(const std::string &buff)
// {
// 	if (buff.empty() || buff.find('\0') != std::string::npos)
// 	{
// 		std::cout << "SOMETHING REALLY BAD HAPPENED" << std::endl;
// 		return ;
// 	}
// 	if (buff.compare(0, 4, "NICK") == 0)
// 	{
// 		std::string nick;
// 		size_t pos = buff.find("\r\n");
// 		if (pos != std::string::npos)
// 		{
// 			nick = buff.substr(pos, buff.length() - pos);
// 			if (nick.length() > 9)
// 			{
// 				std::string errMsg = "client " + nick + " :Erroneous nickname";
// 				Commands	cmds;
				
// 				cmds.sendNumericReply(*this, errMsg);
// 				std::cout << "ERRONEOUS NICKNAME" << std::endl;
// 			}
// 			//check for uniqueness too
// 			this->nickname = nick;
// 		}
// 		else
// 			std::cout << "invalid NICK command" << std::endl;
// 	}
// 	// if (buff.compare(0, 4, "USER") == 0)
// 	// {

// 	// }
// }