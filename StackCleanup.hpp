// StackCleanup.hpp

#pragma once

#include <functional>
#include <stack>

class StackCleanup {
public:
    StackCleanup() = default;
    ~StackCleanup();

    void addCleanupTask(std::function<void()> func);
    void executeAll();

private:
    std::stack<std::function<void()>> cleanupStack;
};
