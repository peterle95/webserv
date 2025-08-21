NAME = webserv
CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98

<<<<<<< HEAD
SRCS = src/main.cpp src/ConfigParser.cpp src/HttpServer.cpp src/ErrorHandler.cpp
=======
SRCS = src/main.cpp \
		src/parser/ConfigParser.cpp \
		src/server/HttpServer.cpp \
		src/parser/Getters.cpp \
		src/parser/Trim.cpp \
		src/error_handling/ErrorHandler.cpp \
		src/error_handling/Getters.cpp
		
>>>>>>> b488101cc1622b45f220c77dc2a260835d9ab971
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