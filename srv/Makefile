CXX = g++
CXXFLAGS = -std=c++11 -DBOOST_BIND_GLOBAL_PLACEHOLDERS -I.

TARGETS = chat_server chat_client

all: $(TARGETS)

chat_server: MainServer.o Server.o PersonInRoom.o ChatRoom.o WorkerThread.o
	$(CXX) $(CXXFLAGS) -o chat_server MainServer.o Server.o PersonInRoom.o ChatRoom.o WorkerThread.o -lpthread -lboost_system -lboost_thread

chat_client: MainClient.o Client.o
	$(CXX) $(CXXFLAGS) -o chat_client MainClient.o Client.o -lpthread -lboost_system

MainServer.o: MainServer.cpp WorkerThread.hpp Server.hpp PersonInRoom.hpp ChatRoom.hpp Participant.hpp Protocol.hpp
	$(CXX) $(CXXFLAGS) -c MainServer.cpp

Server.o: Server.cpp Server.hpp PersonInRoom.hpp ChatRoom.hpp Participant.hpp
	$(CXX) $(CXXFLAGS) -c Server.cpp

PersonInRoom.o: PersonInRoom.cpp PersonInRoom.hpp ChatRoom.hpp Participant.hpp Protocol.hpp
	$(CXX) $(CXXFLAGS) -c PersonInRoom.cpp

ChatRoom.o: ChatRoom.cpp ChatRoom.hpp Participant.hpp Protocol.hpp Utils.hpp
	$(CXX) $(CXXFLAGS) -c ChatRoom.cpp

WorkerThread.o: WorkerThread.cpp WorkerThread.hpp
	$(CXX) $(CXXFLAGS) -c WorkerThread.cpp

MainClient.o: MainClient.cpp Client.hpp Protocol.hpp
	$(CXX) $(CXXFLAGS) -c MainClient.cpp

Client.o: Client.cpp Client.hpp Protocol.hpp
	$(CXX) $(CXXFLAGS) -c Client.cpp

clean:
	rm -f *.o $(TARGETS)


# CC      = g++
# CFLAGS  = -O3
# OPTION  = -std=c++14
# LIBS    = -lboost_system -lboost_thread -pthread
# SRC = chat_server.cpp chat_client.cpp ChatRoom.cpp

# all: chat_server chat_client

# chat_server: chat_server.o
# 	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

# chat_client: chat_client.o
# 	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

# chat_server.o: chat_server.cpp protocol.hpp
# 	$(CC) $(OPTION) -c $(CFLAGS) $(SRC)

# chat_client.o: chat_client.cpp protocol.hpp
# 	$(CC) $(OPTION) -c $(CFLAGS) chat_client.cpp

# .PHONY: clean

# clean:
# 	rm *.o
# 	rm chat_server chat_client
