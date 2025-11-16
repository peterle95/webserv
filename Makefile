NAME = webserv
CXX = c++
CXXFLAGS = -g -Wall -Wextra -Werror -std=c++98 -Iinclude
#CXXFLAGS = -g3 -O0 -DDEBUG=1 -Wall -Wextra -Werror -std=c++17 -Iinclude

SRCS = src/main.cpp \
		src/configParser/ConfigParser.cpp \
		src/configParser/serverConfig/HandleLocationDirective.cpp \
		src/server/HttpServer.cpp \
		src/configParser/Getters.cpp \
		src/configParser/Trim.cpp \
		src/error_handling/ErrorHandler.cpp \
		src/error_handling/Getters.cpp \
		src/configParser/ParserUtils.cpp \
		src/configParser/ParserHelpers.cpp \
		src/configParser/directives/ParseRoot.cpp \
		src/configParser/directives/ParseIndex.cpp \
		src/configParser/directives/ParseClientMaxBodySize.cpp \
		src/configParser/serverConfig/ServerConfig.cpp \
		src/configParser/serverConfig/ParseListen.cpp \
		src/configParser/serverConfig/ParseServerName.cpp \
		src/configParser/serverConfig/ParseAllowedMethods.cpp \
		src/configParser/serverConfig/ParseErrorPage.cpp \
		src/configParser/serverConfig/ParserHelpers.cpp \
		src/httpParser/HTTPparser.cpp \
		src/httpParser/HTTPutils.cpp \
		src/httpParser/HTTPmessageComponents/HTTPHeaders.cpp \
		src/httpParser/HTTPmessageComponents/HTTPRequestLine.cpp \
		src/httpParser/HTTPmessageComponents/HTTPValidation.cpp \
		src/httpParser/HTTPmessageComponents/HTTPBody.cpp \
		src/server/ServerUtils.cpp \
		src/server/BindSocket.cpp \
		src/server/AcceptSocket.cpp \
		src/Client/HandleClient.cpp \
		src/Client/Client.cpp \
		src/CGI/cgi.cpp \
		src/httpResponse/HttpResponse.cpp \
		src/httpResponse/HttpResponseUtils.cpp \
		src/Logging/Logger.cpp \

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