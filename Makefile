
PROJECT=Loup
OBJ=$(HOME)/obj/x64/Debug/$(PROJECT)
BINOUT=$(HOME)/deploy/bin

CC=g++
INCLUDE=-I$(HOME)/include/Xelite -I$(HOME)/include/cardac
sobjects=$(patsubst %.cpp,$(OBJ)/%.o,$(wildcard *.cpp))

vpath %.cpp .
vpath %.o $(OBJ)

$(OBJ)/%.o: %.cpp
	-mkdir -p $(OBJ)
	$(CC) $(INCLUDE) -std=c++0x -c $< -o $(OBJ)/$*.o


$(BINOUT)/$(PROJECT): $(sobjects)
	-mkdir -p $(BINOUT)
	$(CC) $(sobjects) -ldl -lbsd -o $(BINOUT)/$(PROJECT)

clean:
	-rm $(sobjects)
	-rm $(BINOUT)/$(PROJECT)

cleanall:
	-rm $(sobjects)
	-rm $(BINOUT)/$(PROJECT)

all: $(BINOUT)/$(PROJECT)
	
