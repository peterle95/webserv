NAME = webserv
CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98

SRCS = src/main.cpp \
		src/parser/ConfigParser.cpp \
		src/server/HttpServer.cpp \
		src/parser/Getters.cpp \
		src/parser/Trim.cpp
		
OBJS = $(SRCS:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re