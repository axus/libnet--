############################
# Makefile for MSYS + MinGW
############################
# GNU C++ Compiler
CC=g++

SRCDIR=src
BUILD=build

#   Your makefile that includes bin.MinGW.mak must define these:
# BIN           binary name
# SRCFILES      .cpp source file names
# LIBS          -L(lib path) and -l(library name) for all paths and lib names
# INCLUDES      -I(include path) for all include paths
# INSTALL_BIN   What path to install to for "make install"
#   Optional:
# LOGFILES      names of log files to clean up with make clean
# DEBUG         "on" to turn on debugging
# MOREFLAGS     Add custom flags to object compile phase


# -mconsole: Create a console application
# -mwindows: Create a GUI application
# -Wl,--enable-auto-import: Let the ld.exe linker automatically import from libraries
LDFLAGS=-Wl,--enable-auto-import -mwindows

#Minimum Windows version: Windows XP, IE 6.01
#CPPFLAGS=-D_WIN32_WINNT=0x0500 -DWINVER=0x0500 -D_WIN32_IE=0x0601 $(MOREFLAGS)
CPPFLAGS=$(MOREFLAGS) -Wno-deprecated

#SRC files in SRCDIR directory
SRC=$(addprefix $(SRCDIR)/, $(SRCFILES))

# Choose object file names from source file names
OBJFILES=$(SRCFILES:.cpp=.o)
OBJ=$(addprefix $(BUILD)/, $(OBJFILES))

# Debug, or optimize
ifeq ($(DEBUG),on)
  CFLAGS=-Wall -g -pg -DDEBUG
else
  # All warnings, optimization level 3
  CFLAGS=-Wall -O3
endif


# Default target of make is "all"
.all: all      
all: $(BUILD) $(BIN)

#Create build directories if needed
$(BUILD): 
	@[ -d $@ ] || mkdir -p $@

# Build object files with chosen options
$(BUILD)/%.o: $(SRCDIR)/%.cpp
	$(CC) $(CFLAGS) $(CPPFLAGS) $(INCLUDES) -o $@ -c $<

# Build executable from objects and libraries to current directory
$(BIN): $(OBJ)
	$(CC) $^ $(CFLAGS) $(LDFLAGS) $(LIBS) -o $@


#Create install directories if needed
$(INSTALL_BIN): 
	@[ -d $@ ] || mkdir -p $@

install: $(INSTALL_BIN)
	cp $(BIN) $(INSTALL_BIN)/

#How to uninstall
uninstall:
	-rm $(INSTALL_BIN)/$(BIN)

# Remove object files and core files with "clean" (- prevents errors from exiting)
RM=rm -f
.clean: clean
clean:
	-$(RM) $(BIN) $(OBJ) core $(LOGFILES)
