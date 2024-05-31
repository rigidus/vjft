// WorkerThread.cpp
#include "WorkerThread.hpp"

std::mutex WorkerThread::m_;

void WorkerThread::Run(std::shared_ptr<boost::asio::io_service> io_service) {
    {
        std::lock_guard<std::mutex> lock(m_);
        std::cout << "[" << std::this_thread::get_id() << "] Thread starts" << std::endl;
    }

    io_service->run();

    {
        std::lock_guard<std::mutex> lock(m_);
        std::cout << "[" << std::this_thread::get_id() << "] Thread ends" << std::endl;
    }
}
