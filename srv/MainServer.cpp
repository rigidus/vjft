// MainServer.cpp
#include "MainClient.hpp"

int main(int argc, char* argv[]) {
    try {
        if (argc < 2) {
            std::cerr << "Usage: chat_server <port> [<port> ...]\n";
            return 1;
        }

        std::shared_ptr<boost::asio::io_service> io_service(
            new boost::asio::io_service);
        boost::shared_ptr<boost::asio::io_service::work> work(
            new boost::asio::io_service::work(*io_service));
        boost::shared_ptr<boost::asio::io_service::strand> strand(
            new boost::asio::io_service::strand(*io_service));

        std::cout << "[" << std::this_thread::get_id() << "] server starts" << std::endl;

        std::list<std::shared_ptr<Server>> servers;
        for (int i = 1; i < argc; ++i) {
            tcp::endpoint endpoint(tcp::v4(), std::atoi(argv[i]));
            std::shared_ptr<Server> a_server(new Server(*io_service, *strand, endpoint));
            servers.push_back(a_server);
        }

        boost::thread_group workers;
        for (int i = 0; i < 1; ++i) {
            boost::thread* t = new boost::thread{
                boost::bind(&WorkerThread::Run, io_service)};
#ifdef __linux__
            // bind cpu affinity for worker thread in linux
            cpu_set_t cpuset;
            CPU_ZERO(&cpuset);
            CPU_SET(i, &cpuset);
            pthread_setaffinity_np(t->native_handle(), sizeof(cpu_set_t), &cpuset);
#endif
            workers.add_thread(t);
        }

        workers.join_all();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
