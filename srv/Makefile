CXX = g++
CXXFLAGS = -std=c++17  -DBOOST_BIND_GLOBAL_PLACEHOLDERS -I.

TARGETS = chat_server chat_client test_crypto

all: $(TARGETS)

chat_server: MainServer.o Server.o PersonInRoom.o ChatRoom.o WorkerThread.o Message.o Log.hpp defs.hpp
	$(CXX) $(CXXFLAGS) -o chat_server MainServer.o Server.o PersonInRoom.o ChatRoom.o WorkerThread.o Message.o -lpthread -lboost_system -lboost_thread -static

chat_client: MainClient.o Client.o Message.o Crypt.o Utils.o Log.hpp defs.hpp
	$(CXX) $(CXXFLAGS) -o chat_client MainClient.o Client.o Message.o Crypt.o Utils.o -lpthread -lboost_system -lssl -lcrypto

test_crypto: test_crypto.o Client.o Message.o Crypt.o Utils.o Log.hpp defs.hpp
	$(CXX) $(CXXFLAGS) -o test_crypto test_crypto.o Client.o Message.o Crypt.o Utils.o -lpthread -lboost_system -lssl -lcrypto



MainServer.o: MainServer.cpp WorkerThread.hpp Server.hpp PersonInRoom.hpp ChatRoom.hpp Participant.hpp Protocol.hpp Log.hpp defs.hpp
	$(CXX) $(CXXFLAGS) -c MainServer.cpp

Server.o: Server.cpp Server.hpp PersonInRoom.hpp ChatRoom.hpp Participant.hpp Protocol.hpp Log.hpp defs.hpp
	$(CXX) $(CXXFLAGS) -c Server.cpp

PersonInRoom.o: PersonInRoom.cpp PersonInRoom.hpp ChatRoom.hpp Participant.hpp Protocol.hpp Log.hpp defs.hpp
	$(CXX) $(CXXFLAGS) -c PersonInRoom.cpp

ChatRoom.o: ChatRoom.cpp ChatRoom.hpp Participant.hpp Utils.hpp Message.hpp Protocol.hpp Log.hpp defs.hpp
	$(CXX) $(CXXFLAGS) -c ChatRoom.cpp

WorkerThread.o: WorkerThread.cpp WorkerThread.hpp Log.hpp defs.hpp
	$(CXX) $(CXXFLAGS) -c WorkerThread.cpp


MainClient.o: MainClient.cpp Client.hpp Protocol.hpp Crypt.hpp Log.hpp defs.hpp
	$(CXX) $(CXXFLAGS) -c MainClient.cpp

Client.o: Client.cpp Client.hpp Protocol.hpp Crypt.hpp Log.hpp defs.hpp
	$(CXX) $(CXXFLAGS) -c Client.cpp

Message.o: Message.cpp Message.hpp defs.hpp Log.hpp defs.hpp
	$(CXX) $(CXXFLAGS) -c Message.cpp


Crypt.o: Crypt.cpp Crypt.hpp defs.hpp Log.hpp defs.hpp
	$(CXX) $(CXXFLAGS) -c Crypt.cpp

test_crypto.o: test_crypto.cpp test_crypto.hpp Crypt.hpp Message.hpp Log.hpp defs.hpp
	$(CXX) $(CXXFLAGS) -c test_crypto.cpp

Utils.o: Utils.cpp Utils.hpp defs.hpp Log.hpp defs.hpp
	$(CXX) $(CXXFLAGS) -c Utils.cpp


.PHONY: clean

clean:
	rm -f *.o
	rm $(TARGETS)
