############################
# Makefile for MSYS + MinGW
############################
# GNU C++ Compiler
CC=g++

SRCDIR=src
BUILD=build

# -mconsole: Create a console application
# -mwindows: Create a GUI application
# -Wl,--enable-auto-import: Let the ld.exe linker automatically import from libraries
LDFLAGS=-mconsole -Wl,--enable-auto-import

#Minimum Windows version: Windows XP, IE 6.01
CPPFLAGS=-D_WIN32_WINNT=0x0500 -DWINVER=0x0500 -D_WIN32_IE=0x0601 $(MOREFLAGS)

#SRC files in SRCDIR directory
SRC=$(addprefix $(SRCDIR)/, $(SRCFILES))

# Choose object file names from source file names
OBJFILES=$(SRCFILES:.cpp=.o)
OBJ=$(addprefix $(BUILD)/, $(OBJFILES))

# Debug, or optimize
ifeq ($(DEBUG),on)
  CFLAGS=-Wall -g -DDEBUG
else
  # All warnings, optimization level 3
  CFLAGS=-Wall -O3
endif


# Default target of make is "all"
.all: all      
all: $(BIN) $(LIBBIN)

# Build object files with chosen options
$(BUILD)/%.o: $(SRCDIR)/%.cpp
	$(CC) $(CFLAGS) $(CPPFLAGS) $(INCLUDES) -o $@ -c $<

# Build library
$(BIN): $(OBJ)
	ar r $@ $^
	ranlib $@
	
# Remove object files and core files with "clean" (- prevents errors from exiting)
RM=rm -f
.clean: clean
clean:
	-$(RM) $(BIN) $(OBJ) core $(LOGFILES)
