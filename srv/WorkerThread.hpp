// WorkerThread.hpp
#ifndef WORKERTHREAD_HPP
#define WORKERTHREAD_HPP

#include <memory>
#include <thread>
#include <mutex>
#include <iostream>
#include <boost/asio.hpp>

class WorkerThread {
public:
    static void Run(std::shared_ptr<boost::asio::io_service> io_service);

private:
    static std::mutex m_;
};

#endif // WORKERTHREAD_HPP
