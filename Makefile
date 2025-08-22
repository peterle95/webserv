NAME = webserv
CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98

SRCS = src/main.cpp \
		src/configParser/ConfigParser.cpp \
		src/server/HttpServer.cpp \
		src/configParser/Getters.cpp \
		src/configParser/Trim.cpp \
		src/error_handling/ErrorHandler.cpp \
		src/error_handling/Getters.cpp \
		src/configParser/ParserHelpers.cpp \
		src/configParser/directives/ParseListen.cpp \
		src/configParser/directives/ParseRoot.cpp \
		src/configParser/directives/ParseIndex.cpp \
		src/configParser/directives/ParseServerName.cpp \
		src/configParser/directives/ParseClientMaxBodySize.cpp \
		src/configParser/directives/ParseAllowedMethods.cpp \
		src/configParser/directives/ParseErrorPage.cpp
		
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