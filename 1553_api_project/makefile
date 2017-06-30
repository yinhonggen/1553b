DIR_INC = ./include
DIR_SRC = ./src
DIR_OBJ = ./obj
DIR_BIN = ./bin
DIR_PUB = ./pub

DLIBS = -lpthread -lrt

DEFS = -D__linux__ -D_DEBUG -DIPCORE 

SRC = $(wildcard ${DIR_SRC}/*.cpp)  
OBJ = $(patsubst %.cpp,${DIR_OBJ}/%.o,$(notdir ${SRC})) 

TARGET = libzynq_1553b.so
BIN_TARGET = ${DIR_BIN}/${TARGET}

CC = arm-xilinx-linux-gnueabi-g++

CFLAGS = -I${DIR_INC}  -I$(DIR_PUB) $(DLIBS)


${BIN_TARGET}:${OBJ}
	#$(CC) -g -o $@ $(OBJ) $(DLIBS)
	#arm-xilinx-linux-gnueabi-ar -rc $@ $(OBJ)
	$(CC) -fPIC -shared -g -o $@ $(OBJ) $(DLIBS)

${DIR_OBJ}/%.o:${DIR_SRC}/%.cpp
	$(info, $(CFLAGS))
	@echo $(CFLAGS)
	$(CC) $(DEFS) $(CFLAGS) -fPIC -g -c  $< -o $@ 
.PHONY:clean
clean:
	rm -rf ${DIR_OBJ}/*.o ${BIN_TARGET}
