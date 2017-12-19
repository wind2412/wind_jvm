CC := g++
CPP_FLAGS := -std=c++11 -g
SRC_DIR := ../src
INCLUDE_DIR := ../include

all : testClassParser testJarLister testRtJarDirectory testNativeFind

testClassParser : testClassParser.cpp $(SRC_DIR)/class_parser.o $(SRC_DIR)/utils.o
	$(CC) $(CPP_FLAGS) -I$(INCLUDE_DIR) -o $@ $^

testJarLister : testJarLister.cpp $(SRC_DIR)/jarLister.o $(SRC_DIR)/utils.o
	$(CC) $(CPP_FLAGS) -I$(INCLUDE_DIR) -o $@ $^ -L/usr/local/Cellar/boost/1.63.0/lib/ -lboost_filesystem -lboost_system

testRtJarDirectory : testRtJarDirectory.cpp $(SRC_DIR)/jarLister.o $(SRC_DIR)/utils.o
	$(CC) $(CPP_FLAGS) -I$(INCLUDE_DIR) -o $@ $^ -L/usr/local/Cellar/boost/1.63.0/lib/ -lboost_filesystem -lboost_system

testNativeFind : testNativeFind.cpp $(SRC_DIR)/native/java_lang_Object.o
	$(CC) $(CPP_FLAGS) -I$(INCLUDE_DIR) -o $@ $^ -L/usr/local/Cellar/boost/1.63.0/lib/ -lboost_filesystem -lboost_system

clean : 
	@rm -rf rt.list sun_src/ bin/* 
	@rm -rf testClassParser testJarLister testRtJarDirectory testNativeFind
	@rm -rf *.dSYM
