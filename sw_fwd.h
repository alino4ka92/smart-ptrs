#pragma once

#include <cstddef>
#include <exception>
#include <utility>

#ifndef BLOCKS
#define BLOCKS

class BaseControlBlock {
public:
    void StrongIncrement() {
        strong_counter_++;
    }
    void StrongDecrement() {
        if (strong_counter_ == 1) {
            Destroy();
        }
        strong_counter_--;
    }
    size_t StrongUseCount() {
        return strong_counter_;
    }

    void WeakIncrement() {
        weak_counter_++;
    }
    void WeakDecrement() {
        weak_counter_--;
    }
    size_t WeakUseCount() {
        return weak_counter_;
    }

    bool CanBeDeleted() {
        return strong_counter_ == 0 && weak_counter_ == 0;
    }

    virtual void* Get() = 0;

    virtual void Destroy() = 0;
    virtual ~BaseControlBlock() {
    }

private:
    size_t strong_counter_ = 1;
    size_t weak_counter_ = 0;
};

template <class T>
class PointingControlBlock : public BaseControlBlock {
public:
    PointingControlBlock(T* ptr) : BaseControlBlock(), ptr_(ptr) {
    }
    void* Get() override {
        return static_cast<void*>(ptr_);
    }

    void Destroy() override {
        delete ptr_;
    }

private:
    T* ptr_;
};

template <class T>
class EmplacingControlBlock : public BaseControlBlock {
public:
    template <typename... Args>
    EmplacingControlBlock(Args&&... args) : BaseControlBlock() {
        new (&buffer_) T(std::forward<Args>(args)...);
    }

    void* Get() override {
        return static_cast<void*>(reinterpret_cast<T*>(&buffer_));
    }

    void Destroy() override {
        reinterpret_cast<T*>(&buffer_)->~T();
    }

private:
    alignas(T) std::byte buffer_[sizeof(T)];
};

class BadWeakPtr : public std::exception {};

template <typename T>
class SharedPtr;

template <typename T>
class WeakPtr;

#endif