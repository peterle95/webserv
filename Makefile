NAME = webserv
CXX = c++
CXXFLAGS = -g -Wall -Wextra -Werror -std=c++98 -Iinclude

SRCS = src/main.cpp \
		src/configParser/ConfigParser.cpp \
		src/configParser/HandleLocationDirective.cpp \
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
		src/configParser/directives/ParseErrorPage.cpp \
		src/httpParser/HTTPparser.cpp \
		src/httpParser/HTTPutils.cpp \
		src/httpParser/HTTPmessageComponents/HTTPHeaders.cpp \
		src/httpParser/HTTPmessageComponents/HTTPRequestLine.cpp \
		src/httpParser/HTTPmessageComponents/HTTPValidation.cpp \
		src/httpParser/HTTPmessageComponents/HTTPBody.cpp \
		src/server/ServerUtils.cpp \
		src/server/BindSocket.cpp \
		src/server/AcceptSocket.cpp \
		src/server/HandleClient.cpp \
		src/CGI/cgi.cpp \
		src/HttpResponse.cpp \
		src/HttpResponseUtils.cpp
		

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