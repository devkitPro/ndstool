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

export PATH		:=	$(DEVKITARM)/bin:$(PATH)
#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
DEBUGFLAGS	:=


UNAME := $(shell uname -s)

CFLAGS	:=	$(DEBUGFLAGS) -Wall -O3
CFLAGS	+=	$(INCLUDE)


LDFLAGS	=	$(DEBUGFLAGS)

ifneq (,$(findstring Darwin,$(UNAME)))
        LDFLAGS         +=
endif



ifneq (,$(findstring MINGW,$(UNAME)))
	PLATFORM	:= win32
	EXEEXT		:= .exe
	CFLAGS		+= -mno-cygwin
	LDFLAGS		+= -mno-cygwin -s
endif

ifneq (,$(findstring CYGWIN,$(UNAME)))
	CFLAGS		+= -mno-cygwin
	LDFLAGS		+= -mno-cygwin -s
	EXEEXT		:= .exe
endif

ifneq (,$(findstring Linux,$(UNAME)))
	LDFLAGS 	+= -static -s
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
LIBDIRS	:= /usr/local /c/cygwin/usr/local

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
BINFILES	:=	default_arm7.bin loadme.bin $(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.bin)))
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
	@make PassMeIncludes
	@make -C DefaultArm7
	@make -C Loader
	@make --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile
	@./ndstool -@ DefaultArm7/default_arm7.bin && echo For official devkitPro releases, add this SHA1 hash of default ARM7 binary to data/arm7_sha1_homebrew.bin and recompile. || echo -n

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
	@make -C DefaultArm7 clean
	@make -C Loader clean
	@rm -fr $(BUILD) $(OUTPUT)

#---------------------------------------------------------------------------------
all: clean $(BUILD)

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
	-@( cd $(OUTPUTDIR); upx -q -9 $(TARGET)$(EXEEXT) )

#---------------------------------------------------------------------------------
# Compile Targets for C/C++
#---------------------------------------------------------------------------------

#---------------------------------------------------------------------------------
%.o : %.cpp
	@echo $(notdir $<)
	@$(CXX) -MMD $(CXXFLAGS) -o $@ -c $<

#---------------------------------------------------------------------------------
%.o : %.c
	@echo $(notdir $<)
	@$(CC) -MMD $(CFLAGS) -o $@ -c $<

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
