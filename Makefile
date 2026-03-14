# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: dgomez-l <dgomez-l@student.42madrid.com>   +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2026/03/14 13:08:26 by dgomez-l          #+#    #+#              #
#    Updated: 2026/03/14 13:50:28 by dgomez-l         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

G = \033[1;32m
Y = \033[1;33m
R = \033[0;31m
NC = \033[0m

CXX_NAME		=	webserv

INCLUDES_DIR	=	includes

INCLUDE_FILES	=	Config.hpp\
					webserv.hpp\

VPATH			=	objects/:\
					includes/:\
					src/:\
					src/config:\

CXX_SRC			=	main.cpp\
					Config.cpp\

CXX_OBJ_DIR		=	objects
CXX_OBJ			=	$(patsubst %.cpp, $(CXX_OBJ_DIR)/%.o, $(CXX_SRC))

CXX				=	c++
CFLAGS			=	-Wall -Wextra -Werror -std=c++98 -g3

all: $(CXX_NAME)

$(CXX_NAME): $(CXX_OBJ_DIR) $(CXX_OBJ)
	@printf "%-119s\r" ""
	@echo "$(Y)---------------------------------------- Compiling  Webserv ----------------------------------------$(NC)"
	@$(CXX) $(CFLAGS) $(CXX_OBJ) -I$(INCLUDES_DIR) -o $(CXX_NAME)
	@echo "$(G)------------------------------------ Webserv Finished Compiling ------------------------------------$(NC)\n"
	@echo "$(G)----------------------------------------------------------------------------------------------------$(NC)"
	@echo "$(G)----------------------------------------- Webserv Is Ready -----------------------------------------$(NC)"
	@echo "$(G)----------------------------------------------------------------------------------------------------$(NC)\n"

$(CXX_OBJ_DIR):
	@echo "$(R)---------------------------------- Object Directory Doesn't Exist ----------------------------------$(NC)"
	@echo "$(Y)------------------------------------ Creating Objects Directory ------------------------------------$(NC)"
	@mkdir -p $(CXX_OBJ_DIR)
	@echo "$(G)-------------------------------------- Objects Directory Done --------------------------------------$(NC)"

$(CXX_OBJ_DIR)/%.o: %.cpp $(INCLUDE_FILES)
	@mkdir -p $(dir $@)
	@$(CXX) $(CFLAGS) -I$(INCLUDES_DIR) -c $< -o $@
	@printf "%-119s\r" ">CPP compiling: ""$(CXX) $(CFLAGS) -c $< -o $@"

clean:
	@rm -rf $(CXX_OBJ_DIR)
	@echo "$(R)--------------------------------------- Objects Well Cleaned ---------------------------------------$(NC)"

fclean: clean
	@rm -f $(CXX_NAME)
	@echo "$(G)----------------------------------------- Webserv Is Clean -----------------------------------------\n$(NC)"

re: fclean all

leaks: all
	clear
	valgrind --leak-check=full --show_leak_kinds=all ./$(CXX_NAME) VALGRIND_CONFIG_FILE

.DEFAULT_GOAL: all

.PHONY: all clean fclean re leaks
