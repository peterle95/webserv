NAME = webserv
CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -g

SRCS = src/main.cpp \
		src/parser/ConfigParser.cpp \
		src/server/HttpServer.cpp \
		src/parser/Getters.cpp \
		src/parser/Trim.cpp \
		src/error_handling/ErrorHandler.cpp \
		src/error_handling/Getters.cpp \
		src/parser/ParserHelpers.cpp \
		src/parser/directives/ParseListen.cpp \
		src/parser/directives/ParseRoot.cpp \
		src/parser/directives/ParseIndex.cpp \
		src/parser/directives/ParseServerName.cpp \
		src/parser/directives/ParseClientMaxBodySize.cpp \
		src/parser/directives/ParseAllowedMethods.cpp \
		src/parser/directives/ParseErrorPage.cpp \
		src/server/Sockets.cpp
		
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