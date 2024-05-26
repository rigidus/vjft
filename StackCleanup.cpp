// StackCleanup.cpp

#include "StackCleanup.hpp"

StackCleanup::~StackCleanup() {
    executeAll();
}

void StackCleanup::addCleanupTask(std::function<void()> func) {
    cleanupStack.push(func);
}

void StackCleanup::executeAll() {
    while (!cleanupStack.empty()) {
        cleanupStack.top()();
        cleanupStack.pop();
    }
}
