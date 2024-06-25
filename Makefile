# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: glambrig <marvin@42.fr>                    +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2024/03/17 10:34:27 by glambrig          #+#    #+#              #
#    Updated: 2024/06/13 12:39:03 by glambrig         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

.PHONY: all re clean fclean NAME

CFLAGS = -lstdc++ -Wall -Wextra -Werror -g -std=c++98

NAME = ircserv

SRC = *.cpp

OBJ = $(SRC:.c=.o)

all: $(NAME)

$(NAME): $(OBJ)
	c++ $(SRC) -o $(NAME) $(CFLAGS)

clean:
	rm -rf *.o

fclean: clean
	rm -rf $(NAME)

re: fclean all
