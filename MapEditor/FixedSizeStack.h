#pragma once

#include <vector>

template <typename T>
class FixedSizeStack {
private:
    std::vector<T> mStack;
    size_t mMaxSize;

public:
    FixedSizeStack(size_t Size) : mMaxSize(Size) {}

    void clear()
    {
        mStack.clear();
    }

    std::vector<T>& elements() {
        return mStack;
    }

    void push(const T& value) {
        if (mStack.size() >= mMaxSize) {
            mStack.erase(mStack.begin());
        }
        mStack.push_back(value);
    }

    void pop() {
        if (!mStack.empty()) {
            mStack.pop_back();
        }
    }

    T& top() {
        return mStack.back();
    }

    bool empty() const {
        return mStack.empty();
    }

    size_t size() const {
        return mStack.size();
    }

    size_t capacity() const {
        return mMaxSize;
    }
};