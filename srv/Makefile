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

.PHONY: clean

clean:
	rm -f *.o $(TARGETS)
	rm chat_server chat_client
