NAME = prog
CC = c++
CPPFLAGS = -Wall -Wextra -Werror -std=c++98

SRC = main.cpp \
	config/ConfigFile.cpp \
	config/ParsingUtils.cpp \
	config/ServerBlock.cpp \
	config/LocationBlock.cpp \
	http/utils.cpp \
	http/Response.cpp \
	http/Request.cpp

OBJ = ${SRC:%.cpp=obj/%.o}

all: $(NAME)

obj/%.o: %.cpp
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) -c $< -o $@

$(NAME): $(OBJ)
	$(CC) $(CPPFLAGS) $(OBJ) -o $(NAME)

clean:
	rm -f $(OBJ)
	rm -rf obj

fclean: clean
	rm -f $(NAME)

re: clean fclean all

.PHONY: all clean fclean re