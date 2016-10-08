DEFINE1=__CACHE_DEBUG  #输出在屏幕的调试
DEFINE=__LOG_DEBUG   #输出在日志文件
CXXPATH=/usr/local/build/gcc-4.9.4/bin/bin/
CXXLIBPATH=/usr/local/build/gcc-4.9.4/bin/lib64/       #G++4.9.4  glibc库的路径
#LIBAIO=/root/cachePool-master/src/libaio-0.3.106/src/  #libaio 库的位置
CXX=$(CXXPATH)g++ #指定g++的版本
CXXFLAGS+=-g -Wall -Wextra -std=c++0x -L$(CXXLIBPATH) -lstdc++  -L$(CXXLIBPATH) -lpthread -O0 -laio  -D$(DEFINE) #-D$(DEFINE1) #-D$(DEFINE1) 
NGX_ROOT=/usr/local/src/nginx-1.11.1
BUILD_DIR=./build/
BIN_DIR=./bin/
LOG_DIR=./Log/
TARGETS=cacheManger
OBJ=main.o  similarlinkedhashmap.o entrance.o  parsexml.o parsehash.o writeDisk.o bitmap.o Log.o
CLEANUP=rm -f $(BIN_DIR)$(TARGETS) $(BUILD_DIR)*.o

all:$(TARGETS)

clean:
		$(CLEANUP)

$(TARGETS):$(OBJ)
		$(CXX) $(CXXFLAGS)  $(BUILD_DIR)*.o -o $(BIN_DIR)$@
main.o:main.cpp entrance.h 
		$(CXX) $(CXXFLAGS)   -c $< -o $(BUILD_DIR)$@
entrance.o:entrance.cpp entrance.h  
		$(CXX) $(CXXFLAGS)  -c $<  -o $(BUILD_DIR)$@
similarlinkedhashmap.o:similarlinkedhashmap.cpp similarlinkedhashmap.h
		$(CXX) $(CXXFLAGS)  -c $<  -o $(BUILD_DIR)$@
parsexml.o:parsexml.cpp parsexml.h
		$(CXX) $(CXXFLAGS)  -c $<  -o $(BUILD_DIR)$@
parsehash.o:parsehash.cpp parsehash.h
		$(CXX) $(CXXFLAGS)  -c $<  -o $(BUILD_DIR)$@
writeDisk.o:writeDisk.cpp writeDisk.h
		$(CXX) $(CXXFLAGS)  -c $<  -o $(BUILD_DIR)$@
bitmap.o:bitmap.cpp bitmap.h
		$(CXX) $(CXXFLAGS)  -c $<  -o $(BUILD_DIR)$@
Log.o:$(LOG_DIR)Log.cpp $(LOG_DIR)Log.h
		$(CXX) $(CXXFLAGS)  -c $<  -o $(BUILD_DIR)$@


install:
		cp $(BIN_DIR)$(TARGETS) ../$(BIN_DIR)

