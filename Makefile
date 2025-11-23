# The program name
NAME = webserver

# The compiler
CXX = c++

# Flags
CXXFLAGS = -Wall -Wextra -Werror -Iincludes -std=c++17
SANITIZE_FLAGS = -fsanitize=address,undefined -g

# The directory for object files

OBJ_DIR = obj
SRC_DIR = srcs
INC_DIR = includes

# Sources files
SRCS = main.cpp \
		$(SRC_DIR)/Server.cpp \
		$(SRC_DIR)/Request.cpp \
		$(SRC_DIR)/Response.cpp \
		$(SRC_DIR)/Config.cpp \
		$(SRC_DIR)/RequestParser.cpp \
		$(SRC_DIR)/ConfigParser.cpp \
		$(SRC_DIR)/utils.cpp \
		$(SRC_DIR)/Router.cpp \
		$(SRC_DIR)/RouterMethods.cpp \
		$(SRC_DIR)/ConfigCheck.cpp \
		$(SRC_DIR)/ResponseHandling.cpp \
		$(SRC_DIR)/Cgi.cpp \

#
OBJS = $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(SRCS))

# Headers files
INCLUDES = -I$(INC_DIR)

# Rules
all: $(NAME)

# Rule to create the object directory if it doesn't exist
$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)/$(SRC_DIR)
	@echo  "$(BCyan)	📁 Created object directory: $(OBJ_DIR)/$(Color_Off)"

# Creating objects files
$(OBJ_DIR)/%.o: %.cpp | $(OBJ_DIR)
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@
	@echo  "$(BGreen)	✅ Compiled $<$(Color_Off)"

$(NAME): $(OBJS)
	@$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)
	@echo  "$(BGreen)	✅ make $(NAME) Completed!$(Color_Off)"

# sanitize compilation
sanitize: clean
	@$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) $(SRCS) -o $(NAME)
	@echo  "$(BPurple)	🩺✅ sanitize make $(NAME) Completed!$(Color_Off)"

# clean just removes all object files and the object directory
clean:
	@$(RM) -r $(OBJ_DIR)
	@echo  "$(BCyan)	🧹 Object files Cleaned!$(Color_Off)"

# fclean calls clean to remove all object files and in addition, also removes the executable file
fclean: clean
	@$(RM) $(NAME)
	@echo  "$(BYellow)	🗑️  Full Clean Completed!$(Color_Off)"

# re runs fclean and all
re: fclean all

# .PHONY is used to indicate that a target is not a real file but a command or routine to be executed.
# For example, running make clean executes the clean routine, even though there is no file named clean.
.PHONY: all clean fclean re sanitize


# COLORS
# Reset
Color_Off	=	\033[0m\

# Regular Colors
Black	=	\033[0;30m
Red		=	\033[0;31m
Green	=	\033[0;32m
Yellow	=	\033[0;33m
Blue	=	\033[0;34m
Purple	=	\033[0;35m
Cyan	=	\033[0;36m
White	=	\033[0;37m

# Bold
BBlack	=	\033[1;30m
BRed	=	\033[1;31m
BGreen	=	\033[1;32m
BYellow	=	\033[1;33m
BBlue	=	\033[1;34m
BPurple	=	\033[1;35m
BCyan	=	\033[1;36m
BWhite	=	\033[1;37m

# Underline
UBlack	=	\033[4;30m
URed	=	\033[4;31m
UGreen	=	\033[4;32m
UYellow	=	\033[4;33m
UBlue	=	\033[4;34m
UPurple	=	\033[4;35m
UCyan	=	\033[4;36m
UWhite	=	\033[4;37m
