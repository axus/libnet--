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
LDFLAGS=-mwindows -Wl,--enable-auto-import

#Minimum Windows version: Windows XP, IE 6.01
CPPFLAGS=-D_WIN32_WINNT=0x0500 -DWINVER=0x0500 -D_WIN32_IE=0x0601 $(MOREFLAGS)

#SRC files in SRCDIR directory
SRC=$(addprefix $(SRCDIR)/, $(SRCFILES))
HPP=$(addprefix $(SRCDIR)/, $(HEADERS))

# Choose object file names from source file names
OBJFILES=$(SRCFILES:.cpp=.o)
OBJ=$(addprefix $(BUILD)/, $(OBJFILES))
BBIN=$(addprefix $(BUILD)/, $(BIN))

# Debug, or optimize
ifeq ($(DEBUG),on)
  CFLAGS=-Wall -g -DDEBUG
else
  # All warnings, optimization level 3
  CFLAGS=-Wall -O3
endif


# Default target of make is "all"
.all: all      
all: $(BBIN) $(LIBBIN)

# Build object files with chosen options
$(BUILD)/%.o: $(SRCDIR)/%.cpp $(SRCDIR)/%.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(INCLUDES) -o $@ -c $<

# Build library
$(BBIN): $(OBJ)
	ar r $@ $^
	ranlib $@

#Create install directories if needed
$(INSTALL_LIB): 
	@[ -d $@ ] || mkdir -p $@
	
$(INSTALL_INC):
	@[ -d $@ ] || mkdir -p $@

UNINSTALL_HPP = $(addprefix $(INSTALL_INC)/, $(HEADERS))

install: $(INSTALL_LIB) $(INSTALL_INC)
	cp $(BBIN) $(INSTALL_LIB)/
	cp $(HPP) $(INSTALL_INC)/

#How to uninstall
uninstall:
	-$(RM) $(INSTALL_LIB)/$(BIN)
	-$(RM) $(UNINSTALL_HPP)

# Remove object files and core files with "clean" (- prevents errors from exiting)
RM=rm -f
.clean: clean
clean:
	-$(RM) $(BBIN) $(OBJ) core $(LOGFILES)
