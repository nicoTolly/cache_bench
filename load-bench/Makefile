CC=g++
AS=nasm
DEBUG=
HWLOC=yes
HUGEPAGES=
AVX=

ifeq ($(DEBUG),yes)
	CXXFLAGS=-Wall  -g -std=c++11 -fopenmp -fpic   
	EXEC= load.db
else
	CXXFLAGS=-Wall -std=c++11 -fpic -fopenmp -O2
	EXEC= load
endif




SRC= load-test.cpp parser.cpp utils.cpp
LDFLAGS= -lpthread -lrt 

ASM_FLAGS=-felf64
ASM_DIR=

ifeq ($(HWLOC), yes)
	CXXFLAGS+= -DHWLOC
	LDFLAGS+=-lhwloc
endif

ifeq ($(HUGEPAGES), yes)
	CXXFLAGS+= -DUSE_HUGE
	LDFLAGS+=-lhugetlbfs
endif

ifeq ($(AVX), yes)
	CXXFLAGS+= -mavx
	CXXFLAGS+= -DUSE_AVX
	ASM_DIR=$(PROJECT_ROOT_DIR)/arch/x86_avx
else
	ASM_DIR=$(PROJECT_ROOT_DIR)/arch/x86_sse
endif

ASM_SRC= kernel.asm get_cycles.asm

PROJECT_ROOT_DIR= $(shell pwd)

DEBUG_OBJECTS = $(addprefix $(PROJECT_ROOT_DIR)/debug_obj/,$(SRC:.cpp=.o))
RELEASE_OBJECTS = $(addprefix $(PROJECT_ROOT_DIR)/release_obj/,$(SRC:.cpp=.o))
DEBUG_OBJECTS += $(addprefix $(PROJECT_ROOT_DIR)/debug_obj/,$(ASM_SRC:.asm=.o))
RELEASE_OBJECTS += $(addprefix $(PROJECT_ROOT_DIR)/release_obj/,$(ASM_SRC:.asm=.o))

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


# Assembly files
$(PROJECT_ROOT_DIR)/release_obj/get_cycles.o: $(ASM_DIR)/get_cycles.asm
	$(AS) $(ASM_FLAGS) -o $@ $<

$(PROJECT_ROOT_DIR)/debug_obj/get_cycles.o: $(ASM_DIR)/get_cycles.asm
	$(AS) $(ASM_FLAGS) -o $@ $<

#ifeq ($(AVX), yes)
$(PROJECT_ROOT_DIR)/release_obj/kernel.o: $(ASM_DIR)/kernel.asm
	$(AS) $(ASM_FLAGS) -o $@ $<

$(PROJECT_ROOT_DIR)/debug_obj/kernel.o: $(ASM_DIR)/kernel.asm
	$(AS) $(ASM_FLAGS) -o $@ $<
#else
#$(PROJECT_ROOT_DIR)/release_obj/kernel.o: $(PROJECT_ROOT_DIR)/sse_kernel.asm
#	$(AS) $(ASM_FLAGS) -o $@ $<
#
#$(PROJECT_ROOT_DIR)/debug_obj/kernel.o: $(PROJECT_ROOT_DIR)/sse_kernel.asm
#	$(AS) $(ASM_FLAGS) -o $@ $<
#endif

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

