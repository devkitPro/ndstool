#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
.SECONDARY:
#---------------------------------------------------------------------------------

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# INCLUDES is a list of directories containing extra header files
#---------------------------------------------------------------------------------
TARGET		:=	ndstool
BUILD		:=	build
SOURCES		:=	source
INCLUDES	:=	include
DATA		:=	data
VERSION		:=	1.41

export PATH		:=	$(MINGW)/bin:$(DEVKITARM)/bin:$(PATH)
#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
DEBUGFLAGS	:=

GAWK	?=	awk


UNAME := $(shell uname -s)

CFLAGS	:=	$(DEBUGFLAGS) -Wall -O2
CFLAGS	+=	$(INCLUDE)


LDFLAGS	=	$(DEBUGFLAGS)

ifneq (,$(findstring MINGW,$(UNAME)))
	PLATFORM	:= win32
	EXEEXT		:= .exe
	CFLAGS		+= -mno-cygwin
	LDFLAGS		+= -mno-cygwin -s
	OS	:=	win32
endif

ifneq (,$(findstring CYGWIN,$(UNAME)))
	CFLAGS		+= -mno-cygwin
	LDFLAGS		+= -mno-cygwin -s
	EXEEXT		:= .exe
	OS	:=	win32
endif

ifneq (,$(findstring Darwin,$(UNAME)))
	OSXCFLAGS	:= -O -mmacosx-version-min=10.4 -isysroot /Developer/SDKs/MacOSX10.4u.sdk -arch i386 -arch ppc
	LDFLAGS += -arch i386 -arch ppc
endif

ifneq (,$(findstring Linux,$(UNAME)))
	LDFLAGS += -s -static
	OS := Linux
endif

CXXFLAGS	=	$(CFLAGS) -fno-rtti -fno-exceptions

#---------------------------------------------------------------------------------
# any extra libraries we wish to link with the project
#---------------------------------------------------------------------------------
LIBS	:=

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS	:= 

#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUTDIR:=	$(CURDIR)
export OUTPUT	:=	$(CURDIR)/$(TARGET)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
			$(foreach dir,$(DATA),$(CURDIR)/$(dir)) \
			$(CURDIR)/DefaultArm7 $(CURDIR)/Loader

export CC	:=	gcc
export CXX	:=	g++
export AR	:=	ar

#---------------------------------------------------------------------------------
# automatically build a list of object files for our project
#---------------------------------------------------------------------------------
CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES	:=	loadme.bin $(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.bin)))
BMPFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.bmp)))

export OFILES	:= $(BINFILES:.bin=.o) $(BMPFILES:.bmp=.o)  $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
#---------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
#---------------------------------------------------------------------------------
	export LD	:=	$(CC)
#---------------------------------------------------------------------------------
else
#---------------------------------------------------------------------------------
	export LD	:=	$(CXX)
#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			-I$(CURDIR)/$(BUILD)

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)

.PHONY: $(BUILD) clean


#---------------------------------------------------------------------------------
$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) PassMeIncludes
	@$(MAKE) -C Loader
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

all: clean $(BUILD)

#---------------------------------------------------------------------------------
.PHONY: PassMeIncludes
PassMeIncludes: $(BUILD)/passme_vhd1.h $(BUILD)/passme_vhd2.h

$(BUILD)/passme_vhd1.h: $(SOURCES)/passme.vhd
	cat $< | $(GAWK) '/^--!/ { COND=1; $$0=""; } // { gsub("\"", "\\\""); gsub("\011", "\\t"); gsub("\015", ""); if (!COND) print "\"" $$0 "\\n\"" }' > $@

$(BUILD)/passme_vhd2.h: $(SOURCES)/passme.vhd
	cat $< | $(GAWK) '/^--!/ { COND=1; $$0=""; } // { gsub("\"", "\\\""); gsub("\011", "\\t"); gsub("\015", ""); if (COND) print "\"" $$0 "\\n\"" }' > $@

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@$(MAKE) -C Loader clean
	@rm -fr $(BUILD) $(OUTPUT)$(EXEEXT)

#---------------------------------------------------------------------------------

dist-src:
	@tar --exclude=*CVS* -cvjf ndstool-src-$(VERSION).tar.bz2 data DefaultArm7 include Loader source Makefile 

dist-bin: all
	@tar -cjvf ndstool-$(VERSION)-$(OS).tar.bz2 $(TARGET)$(EXEEXT)

dist: dist-src dist-bin

#---------------------------------------------------------------------------------
run: $(OUTPUT)
	@echo $(OUTPUT)

install:
	cp  $(OUTPUT)$(EXEEXT) $(PREFIX)

#---------------------------------------------------------------------------------
else

DEPENDS	:=	$(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
$(OUTPUT): $(OFILES)
	@echo linking
	@$(LD) $(LDFLAGS) $(OFILES) $(LIBPATHS) $(LIBS) -o $(OUTPUT)$(EXEEXT)

#---------------------------------------------------------------------------------
# Compile Targets for C/C++
#---------------------------------------------------------------------------------


#---------------------------------------------------------------------------------
ndstool.o : ndstool.cpp $(OUTPUTDIR)/Makefile
	@echo $(notdir $<)
	$(CXX) -E -MMD $(CFLAGS) $< > /dev/null
	$(CXX) -DVERSION=\"$(VERSION)\" $(OSXCFLAGS) $(CFLAGS) -o $@ -c $<

#---------------------------------------------------------------------------------
%.o : %.cpp
	@echo $(notdir $<)
	$(CXX) -E -MMD $(CFLAGS) $< > /dev/null
	$(CXX) $(OSXCFLAGS) $(CFLAGS) -o $@ -c $<

#---------------------------------------------------------------------------------
%.o : %.c
	@echo $(notdir $<)
	$(CC) -E -MMD $(CFLAGS) $< > /dev/null
	$(CC) $(OSXCFLAGS) $(CFLAGS) -o $@ -c $<

#---------------------------------------------------------------------------------
%.o : %.s
	@echo $(notdir $<)
	@$(CC) -MMD $(ASFLAGS) -o $@ -c $<


#---------------------------------------------------------------------------------
%.c	:	%.bmp
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@raw2c $<

#---------------------------------------------------------------------------------
%.c	:	%.bin
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@raw2c $<

-include $(DEPENDS)

#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------
