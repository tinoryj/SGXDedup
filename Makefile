CC?=gcc
CXX?=g++

#set include and link path/.so
BOOST_LIBRARY_PATH?=/home/publc/lib
BOOST_INCLUDE_PATH?=/home/public/include
BOOST_LIBRARY_FLAG?=-lboost_thread -lboost_system -lboost_serialization -lrt 

OPENSSL_INCLUDE_PATH?=/home/public/lib/openssl/include/openssl

CLIENT_INCLUDE_PATH?=$(BOOST_INCLUDE_PATH) src
CLIENT_LIBRARY_PATH?=$(BOOST_LIBRARY_PATH)
CLIENT_LIBRARY_FLAG?=$(BOOST_LIBRARY_FLAG)

KEYMANGER_INCLUDE_PATH?=$(BOOST_INCLUDE_PATH) src
KEYMANGER_LIBRARY_PATH?= $(BOOST_LIBRARY_PATH)
KEYMANGER_LIBRARY_FLAG?=$(BOOST_LIBRARY_FLAG)

CLIENT_OBJECT= chunk.o _chunker.o _encoder.o configure.o ssl.o _keyManager.o

vpath %.cpp src

all:$(CLIENT_OBJECT)

chunk.o:chunk.cpp
	@$(CXX) $^ -o $@ -c
	@echo "CXX GEN => $@"

_chunker.o:_chunker.cpp
	@$(CXX) $^ -o $@ -c -I $(BOOST_INCLUDE_PATH)
	@echo "CXX GEN => $@"

_encoder.o:_encoder.cpp
	@$(CXX) $^ -o $@ -c -I $(BOOST_INCLUDE_PATH)
	@echo "CXX GEN => $@"

configure.o:configure.cpp
	@$(CXX) $^ -o $@ -c -I $(BOOST_INCLUDE_PATH)
	@echo "CXX GEN => $@"

ssl.o::ssl.cpp
	@$(CXX) $^ -o $@ -c -I $(OPENSSL_INCLUDE_PATH)
	@echo "CXX GEN => $@"

_keyManager.o:_keyManager.cpp
	@$(CXX) $^ -o $@ -c -I $(BOOST_INCLUDE_PATH)
	@echo "CXX GEN => $@"

#########################################

_messageQueue.o:_messageQueue.cpp
	@$(CXX) $^ -o $@ -c -I $(BOOST_INCLUDE_PATH)
	@echo "CXX GEN => $@"

keyClient.o:keyClient.cpp
	@$(CXX) $^ -o $@ -c -I $(BOOST_INCLUDE_PATH)
	@echo "CXX GEN => $@"

keyServer.o:keyServer.cpp
	@$(CXX) $^ -o $@ -c -I $(BOOST_INCLUDE_PATH)
	@echo "CXX GEN => $@"

seriazation.o:seriazation.cpp
	@$(CXX) $^ -o $@ -c -I $(BOOST_INCLUDE_PATH) -I $(OPENSSL_INCLUDE_PATH)
	@echo "CXX GEN => $@"

chunker.o:chunker.cpp
	@$(CXX) $^ -o $@ -c -I $(BOOST_INCLUDE_PATH)
	@echo "CXX GEN => $@"

_CryptoPrimitive.o:_CryptoPrimitive.cpp
	@$(CXX) $^ -o $@ -c -I $(OPENSSL_INCLUDE_PATH)
	@echo "CXX GEN => $@"


clean:
	@-rm $(CLIENT_OBJECT)