CC=g++
DEBUG=
HWLOC=
HUGEPAGES=

ifeq ($(DEBUG),yes)
	CXXFLAGS=-Wall -g -std=c++11 -fopenmp -fpic  -mavx 
	EXEC= load.db
else
	CXXFLAGS=-Wall -std=c++11 -fpic -fopenmp -mavx -O3
	EXEC= load
endif




SRC= load-test.cpp utils.cpp
LDFLAGS= -lpthread -lrt 

ifeq ($(HWLOC), yes)
	CXXFLAGS+= "-DHWLOC"
	LDFLAGS+=-lhwloc
endif

ifeq ($(HUGEPAGES), yes)
	CXXFLAGS+= "-DUSE_HUGE"
	LDFLAGS+=-lhugetlbfs
endif

PROJECT_ROOT_DIR= $(shell pwd)

DEBUG_OBJECTS = $(addprefix $(PROJECT_ROOT_DIR)/debug_obj/,$(SRC:.cpp=.o))
RELEASE_OBJECTS = $(addprefix $(PROJECT_ROOT_DIR)/release_obj/,$(SRC:.cpp=.o))


.PHONY: all
.PHONY: debug
.PHONY: clean

all: $(EXEC)

debug: $(EXECD)
	$(CXXFLAGS)+=-g



load.db:$(DEBUG_OBJECTS)
	$(CC) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

load: $(RELEASE_OBJECTS)
	$(CC) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(PROJECT_ROOT_DIR)/release_obj/%.o: %.cpp
	$(CC) $(CXXFLAGS) -o $@ -c $<


$(PROJECT_ROOT_DIR)/debug_obj/%.o: %.cpp
	$(CC) $(CXXFLAGS) -o $@ -c $<

depend:.depend

.depend: $(SRC)
	rm -f ./.depend
	$(CC) $(CXXFLAGS) -MM $^ | sed -e "s:\(.*\.o\):$(PROJECT_ROOT_DIR)/release_obj/\1:g" >> ./.depend
	$(CC) $(CXXFLAGS) -MM $^ | sed -e "s:\(.*\.o\):$(PROJECT_ROOT_DIR)/debug_obj/\1:g" >> ./.depend


include .depend


clean:
	rm -fr $(PROJECT_ROOT_DIR)/release_obj/*.o
	rm -fr $(PROJECT_ROOT_DIR)/debug_obj/*.o

mrproper: clean
	rm -fr load load.db

