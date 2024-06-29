// MainClient.cpp
#include <iostream>
#include <thread>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include "MainClient.hpp"

using boost::asio::ip::tcp;

// Declared in Client.cpp
extern std::string client_private_key_file;
extern std::vector<std::string> recipient_public_key_files;


int main(int argc, char* argv[]) {
    try {
        if (argc < 5) {
            std::cerr << "Usage: chat_client <nickname> <host> <port> <client_priv_key_file> <recipient_pub_key_file_1> [<recipient_pub_key_file_2> ...]\n";
            return 1;
        }

        client_private_key_file = argv[4];
        for (int i = 5; i < argc; ++i) {
            recipient_public_key_files.push_back(argv[i]);
        }

        boost::asio::io_service io_service;
        tcp::resolver resolver(io_service);
        tcp::resolver::query query(argv[2], argv[3]);
        tcp::resolver::iterator iterator = resolver.resolve(query);
        std::array<char, MAX_NICKNAME> nickname;
        strcpy(nickname.data(), argv[1]);

        LOG_TXT("Creating client instance");
        Client cli(nickname, io_service, iterator);

        LOG_TXT("MainClient::main(): Starting IO service thread");
        std::thread t(boost::bind(&boost::asio::io_service::run, &io_service));

        // std::array<char, MAX_PACK_SIZE> msg;
        std::string msg;

        while (true) {
            if (std::getline(std::cin, msg) && !msg.empty()) {
                LOG_TXT("Msg size: " << msg.size());
                std::vector<unsigned char> msg_vec(
                    msg.begin(), msg.begin() + strlen(msg.data()));
                LOG_TXT("Sending message: ["
                        << std::string(msg_vec.begin(), msg_vec.end())
                        << "]");
                cli.Write(msg_vec);
            } else {
                std::cin.clear(); //clean up error bit and try to finish reading
            }
        }

        // Uncomment the following and comment out the above while(true), for testing purpose
        /*
          char line[MAX_PACK_SIZE - 32] = "This is a testing line to see if any splits.";
          int i = 0;
          while (i < 1000) {
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
          memset(msg.data(), '\0', MAX_PACK_SIZE);
          strcpy(msg.data(), std::to_string(i).c_str());
          strcat(msg.data(), std::string(" ").c_str());
          strcat(msg.data(), line);
          cli.Write(msg);
          ++i;
          }
          std::cout << "finished" << std::endl;
        */

        LOG_TXT("Main: Closing client");
        cli.Close();
        t.join();
    } catch (std::exception& e) {
        LOG_TXT("Exception: " << e.what());
    }

    return 0;
}
