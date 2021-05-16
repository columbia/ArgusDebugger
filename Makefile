TARGET     := argus
UNAME      := $(shell uname)
CXX        := /usr/bin/clang++
CC         := /usr/bin/clang
CFLAGS        := -c -Wall
BOOSTPATH := /usr/local/Cellar/boost/
PYTHONPATH := /System/Library/Frameworks/Python.framework/Versions/Current

ifeq ($(UNAME), Linux)
	CXXFLAGS    := -c -Wall -std=c++11 -stdlib=libstdc++ 
	LDFLAGS		:= -L /usr/include/boost/ -lboost_thread\
					-lboost_regex -lboost_filesystem -lboost_system\
					-lpthread -ldl\
					-headerpad_max_install_names\
					-L /opt/lib/python2.7/config -lutil -lm -lpython2.7 -Xlinker -export-dynamic
	INC_BOOST	:= -I/usr/include/boost/
	INC_PYTHON  := -I/opt/include/python2.7 -fno-strict-aliasing -DNDEBUG -g -fwrapv -O3 -Wall -Wstrict-prototypes
endif

ifeq ($(UNAME), Darwin)
	LLDBPATH := /Applications/Xcode.app/Contents/SharedFrameworks/LLDB.framework
	INC_LLDB	:= -I lldbHeaders
	CXXFLAGS    := -c -Wall -std=c++11 -stdlib=libc++ 
	LDFLAGS     := -Xlinker $(LLDBPATH)/LLDB \
            		-L$(BOOSTPATH)/lib \
            		-lboost_regex-mt -lboost_filesystem-mt -lboost_thread-mt -lboost_system-mt\
					-lpthread -ldl\
					-headerpad_max_install_names\
					-u _PyMac_Error $(PYTHONPATH)/python
	INC_BOOST    := -I$(BOOSTPATH)/include/
	INC_PYTHON   := -I$(PYTHONPATH)/include/python2.7/
endif

MODULES        := TracingEvents Loader Parser GraphsGenerator Canonization
MODULES        += GraphsGenerator/Connectors GraphsGenerator/VoucherTrace GraphsGenerator/Dividers
INC_DIR        := -I$(PWD)/include/ $(addprefix -I$(PWD)/include/, $(MODULES))
SRC_DIR        := $(addprefix $(PWD)/src/, $(MODULES))
OBJ_DIR        := $(PWD)/obj/utils $(PWD)/obj/shell $(addprefix $(PWD)/obj/, $(MODULES)) $(PWD)/obj
BIN_DIR        := $(PWD)/bin/
CXX_SOURCES    := $(foreach sdir, $(SRC_DIR), $(wildcard $(sdir)/*.cpp))
C_SOURCES     := $(foreach sdir, $(SRC_DIR), $(wildcard $(sdir)/*.c))
CXX_OBJECTS     := $(patsubst $(PWD)/src/%.cpp, $(PWD)/obj/%.o, $(CXX_SOURCES)) 
C_OBJECTS     := $(patsubst $(PWD)/src/%.c, $(PWD)/obj/%.o, $(C_SOURCES))
EXECUTABLE    := $(addprefix $(BIN_DIR), $(TARGET))

#build extra utils upon graphs
CXX_OBJECTS += $(patsubst $(PWD)/utils/%.cpp, $(PWD)/obj/utils/%.o, $(wildcard $(PWD)/utils/*.cpp)) 
INC_DIR    += -I$(PWD)/utils/

#build extra shell upon graphs
CXX_OBJECTS += $(patsubst $(PWD)/shell/%.cpp, $(PWD)/obj/shell/%.o, $(wildcard $(PWD)/shell/*.cpp)) 
INC_DIR    += -I$(PWD)/shell/

INCLUDES    := $(INC_LLDB) $(INC_BOOST) $(INC_DIR) $(INC_PYTHON)
vpath %.cpp $(SRC_DIR) $(PWD)/src/ $(PWD)/utils/ $(PWD)/shell/
vpath %.c $(SRC_DIR) $(PWD)/src/ $(PWD)/utils/ $(PWD)/shell/

define make-goal-cxx
$1/%.o: %.cpp
	$(CXX) $(INCLUDES) $(CXXFLAGS) $$< -o $$@
endef

define make-goal-c
$1/%.o: %.c
	$(CC) $(INCLUDES) $(CFLAGS) $$< -o $$@
endef

.PHONY:directories

all: directories $(EXECUTABLE)
directories: $(OBJ_DIR) $(BIN_DIR)
	@echo $(UNAME)

$(OBJ_DIR):
	mkdir -p $@
$(BIN_DIR):
	mkdir -p $@


define make-target
$1: $(PWD)/obj/$(notdir $1).o $(CXX_OBJECTS) $(C_OBJECTS)
	$(CXX) $(INCLUDES) $(CXX_OBJECTS) $(C_OBJECTS) $(LDFLAGS) $$< -o $$@ 
ifeq ($(UNAME), Darwin)
	install_name_tool -change @rpath/LLDB.framework/LLDB /Applications/Xcode.app/Contents/SharedFrameworks/LLDB.framework/LLDB $$@
	install_name_tool -change @rpath/LLDB.framework/Versions/A/LLDB /Applications/Xcode.app/Contents/SharedFrameworks/LLDB.framework/LLDB $$@
endif
endef


$(foreach target, $(EXECUTABLE), $(eval $(call make-target, $(target))))
$(foreach bdir, $(OBJ_DIR), $(eval $(call make-goal-cxx, $(bdir))))
$(foreach bdir, $(OBJ_DIR), $(eval $(call make-goal-c, $(bdir))))

clean:
	rm -rf obj
	rm -rf bin
