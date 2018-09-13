CC := g++
CPP_FLAGS := -std=c++14 -O3 -pg
#CPP_FLAGS := -std=c++14 -O3 -pg -DBYTECODE_DEBUG -DDEBUG
# CPP_FLAGS := -std=c++14 -O3 -DDEBUG -DKLASS_DEBUG -DPOOL_DEBUG -DBYTECODE_DEBUG
LINK_FLAGS := -std=c++14 -pg
EXCEPT := ./useful_tools/classfile_interceptor.cpp
# CPP_SOURCE := $(filter-out $(EXCEPT), $(shell find . -path "./tests" -prune -o -maxdepth 2 -name "*.cpp" -print))
# CPP_SOURCE := $(filter-out $(EXCEPT), $(shell find ./src -name "*.cpp"))
CPP_SOURCE := $(filter-out $(EXCEPT), $(shell find ./src -name "*.cpp"))
JAVA_TEST_SOURCE := $(shell find . -name "*.java")
CPP_OBJ := $(patsubst %.cpp, %.o, $(CPP_SOURCE))
JAVA_TEST_OBJ := $(patsubst %.java, %.class, $(JAVA_TEST_SOURCE))

.cpp.o:
ifeq ($(shell uname),Darwin)
	$(CC) $(CPP_FLAGS) -g -I./include -c $< -o $@
else 
	$(CC) $(CPP_FLAGS) -g -I./include -c $< -o $@
endif

%.class: %.java
	javac -cp . $<

all : $(CPP_OBJ)
ifeq ($(shell uname),Darwin)
#	$(CC) $(CPP_FLAGS) -L/usr/local/Cellar/boost/1.60.0_2/lib/ -lboost_filesystem -lboost_system -lboost_regex -g -I./include -o bin/wind_jvm $^ $(EXCEPT) // 这个在 clang++ 上是对的!
	$(CC) $(LINK_FLAGS) -L/usr/local/Cellar/boost/1.60.0_2/lib/ -g -I./include -o bin/wind_jvm $^ -lboost_filesystem -lboost_system
else 
	$(CC) $(LINK_FLAGS) -L/usr/lib/x86_64-linux-gnu/ -pthread -g -I./include -o bin/wind_jvm $^ -lboost_system -lboost_filesystem
endif

test : $(JAVA_TEST_OBJ)
	@cd tests && make all

interceptor: $(EXCEPT) src/class_parser.o
	$(CC) $(CPP_FLAGS) -g -DDEBUG -DKLASS_DEBUG -DPOOL_DEBUG -I./include $^ -o useful_tools/$@

clean : 
	@rm -rf rt.list bin/* *.dSYM && cd tests && make clean
	@find . -name "*.o" | xargs rm -f
#	@rm -rf *.class
#	@rm -rf sun_src/
