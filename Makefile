CC := g++
CPP_FLAGS := -std=c++14 -O3 -pg
#CPP_FLAGS := -std=c++14 -pg
#CPP_FLAGS := -std=c++14 -O3 -pg
#CPP_FLAGS := -std=c++14 -DNDEBUG -O3 -pg
#CPP_FLAGS := -std=c++14 -DBYTECODE_DEBUG
#CPP_FLAGS := -std=c++14 -DBYTECODE_DEBUG -DDEBUG
LINK_FLAGS := -std=c++14
LINK_FLAGS := -std=c++14 -pg
#CPP_FLAGS := -std=c++14 -DDEBUG -DKLASS_DEBUG -DPOOL_DEBUG	-DSTRING_DEBUG
# EXCEPT := ./src/main.cpp
# CPP_SOURCE := $(filter-out $(EXCEPT), $(shell find . -path "./tests" -prune -o -maxdepth 2 -name "*.cpp" -print))
# CPP_SOURCE := $(filter-out $(EXCEPT), $(shell find ./src -name "*.cpp"))
CPP_SOURCE := $(shell find ./src -name "*.cpp")
CPP_OBJ := $(patsubst %.cpp, %.o, $(CPP_SOURCE))

.cpp.o:
ifeq ($(shell uname),Darwin)
	$(CC) $(CPP_FLAGS) -g -I./include -c $< -o $@
else 
	$(CC) $(CPP_FLAGS) -g -I./include -c $< -o $@
endif

all : $(CPP_OBJ)
ifeq ($(shell uname),Darwin)
#	$(CC) $(CPP_FLAGS) -L/usr/local/Cellar/boost/1.60.0_2/lib/ -lboost_filesystem -lboost_system -lboost_regex -g -I./include -o bin/wind_jvm $^ $(EXCEPT) // 这个在 clang++ 上是对的!
	$(CC) $(LINK_FLAGS) -L/usr/local/Cellar/boost/1.60.0_2/lib/ -g -I./include -o bin/wind_jvm $^ -lboost_filesystem -lboost_system
else 
	$(CC) $(LINK_FLAGS) -L/usr/lib/x86_64-linux-gnu/ -pthread -g -I./include -o bin/wind_jvm $^ -lboost_system -lboost_filesystem
endif
# 卧槽？？ g++ 必须把链接库放在最后边？？？？放在前面能识别但是不装载......mac 的 ld 就没这个问题啊......

test :
	@cd tests && make all

clean : 
	@rm -rf rt.list bin/* *.dSYM && cd tests && make clean
	@find . -name "*.o" | xargs rm -f
#	@rm -rf sun_src/
