# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: dgomez-l <dgomez-l@student.42madrid.com>   +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2026/03/14 13:08:26 by dgomez-l          #+#    #+#              #
#    Updated: 2026/06/20 14:57:31 by dgomez-l         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

GREEN			=	\033[0;32m
RED				=	\033[0;31m\033[1m
ORANGE			=	\001\033[38;5;208m\002
BLUE			=	\033[0;34m
PURPLE			=	\033[0;35m
CYAN			=	\033[0;36m
YELLOW			=	\033[33m
ROSE			=	\x1B[38;2;255;151;203m
LIGHT_BLUE		=	\x1B[38;2;53;149;240m
LIGHT_GREEN		=	\x1B[38;2;17;245;120m
GRAY			=	\x1B[38;2;176;174;174m
NC				=	\033[0m

CXX_NAME		=	webserv

INCLUDES_FLAG	=	-I includes -I includes/config

INCLUDE_FILES	=	Config.hpp\
					Webserver.hpp\
					Server.hpp\
					Client.hpp\
					HttpRequest.hpp\


VPATH			=	objects/:\
					includes/:\
					includes/config/:\
					src/:\
					src/config/:\
					src/webManager/:\

CXX_SRC			=	main.cpp\
					Client.cpp \
					Server.cpp \
					sendWebPage.cpp\
					Upload.cpp\
					showUploads.cpp \
					handleGet.cpp \
					handlePost.cpp \
					handleCgi.cpp \
					webChecker.cpp \
					parse_request.cpp\
					handleRequests.cpp


CXX_OBJ_DIR		=	objects
CXX_OBJ			=	$(patsubst %.cpp, $(CXX_OBJ_DIR)/%.o, $(CXX_SRC))

CXX				=	c++
CFLAGS			=	-Wall -Wextra -Werror -std=c++98 -g3

all: $(CXX_NAME)

$(CXX_NAME): $(CXX_OBJ_DIR) $(CXX_OBJ)
	@printf "%-119s\r" ""
	@echo "$(YELLOW)---------------------------------------- Compiling  Webserv ----------------------------------------$(NC)"
	@$(CXX) $(CFLAGS) $(CXX_OBJ) $(INCLUDES_FLAG) -o $(CXX_NAME)
	@echo "$(GREEN)------------------------------------ Webserv Finished Compiling ------------------------------------$(NC)\n"
	@echo "$(GREEN)----------------------------------------------------------------------------------------------------$(NC)"
	@echo "$(GREEN)----------------------------------------- Webserv Is Ready -----------------------------------------$(NC)"
	@echo "$(GREEN)----------------------------------------------------------------------------------------------------$(NC)\n"

$(CXX_OBJ_DIR):
	@echo "$(RED)---------------------------------- Object Directory Doesn't Exist ----------------------------------$(NC)"
	@echo "$(YELLOW)------------------------------------ Creating Objects Directory ------------------------------------$(NC)"
	@mkdir -p $(CXX_OBJ_DIR)
	@echo "$(GREEN)-------------------------------------- Objects Directory Done --------------------------------------$(NC)"

$(CXX_OBJ_DIR)/%.o: %.cpp $(INCLUDE_FILES)
	@mkdir -p $(dir $@)
	@$(CXX) $(CFLAGS) -c $< -o $@ $(INCLUDES_FLAG)
	@printf "%-119s\r" ">CPP compiling: ""$(CXX) $(CFLAGS) -c $< -o $@"

clean:
	@rm -rf $(CXX_OBJ_DIR)
	@echo "$(RED)--------------------------------------- Objects Well Cleaned ---------------------------------------$(NC)"

fclean: clean
	@rm -f $(CXX_NAME)
	@echo "$(GREEN)----------------------------------------- Webserv Is Clean -----------------------------------------\n$(NC)"

re: fclean all

leaks: all
	clear
	valgrind --leak-check=full --show_leak_kinds=all ./$(CXX_NAME) VALGRIND_CONFIG_FILE

.DEFAULT_GOAL: all

.PHONY: all clean fclean re leaks
