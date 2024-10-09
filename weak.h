#pragma once

#include "shared.h"
#include "sw_fwd.h"  // Forward declaration

#ifndef WEAK
#define WEAK

class BaseEnableSharedFromThis;

// https://en.cppreference.com/w/cpp/memory/weak_ptr
template <typename T>
class WeakPtr {
public:
    template <class Y>
    friend class WeakPtr;
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    WeakPtr() = default;

    template <class Y>
    WeakPtr(const WeakPtr<Y>& other) : block_(other.GetBlock()) {
        Increment();
    }

    WeakPtr(const WeakPtr& other) : block_(other.GetBlock()) {
        Increment();
    }

    template <class Y>
    WeakPtr(WeakPtr<Y>&& other) : block_(other.GetBlock()) {
        other.block_ = nullptr;
    }

    WeakPtr(WeakPtr&& other) : block_(other.GetBlock()) {
        other.block_ = nullptr;
    }

    // Demote `SharedPtr`
    // #2 from https://en.cppreference.com/w/cpp/memory/weak_ptr/weak_ptr
    template <class Y>
    WeakPtr(const SharedPtr<Y>& other) : block_(other.GetBlock()) {
        Increment();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    template <class Y>
    WeakPtr& operator=(const SharedPtr<Y>& other) {
        Decrement();
        block_ = other.GetBlock();
        Increment();

        return *this;
    }

    template <class Y>
    WeakPtr& operator=(const WeakPtr<Y>& other) {
        Decrement();
        block_ = other.block_;
        Increment();

        return *this;
    }

    WeakPtr& operator=(const WeakPtr& other) {
        Decrement();
        block_ = other.block_;
        Increment();

        return *this;
    }

    template <class Y>
    WeakPtr& operator=(WeakPtr<Y>&& other) {
        Decrement();
        block_ = other.block_;
        Increment();
        other.block_ = nullptr;

        return *this;
    }

    WeakPtr& operator=(WeakPtr&& other) {
        Decrement();
        block_ = other.block_;

        other.block_ = nullptr;

        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~WeakPtr() {
        Decrement();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void Reset() {
        Decrement();
        block_ = nullptr;
    }
    void Swap(WeakPtr& other) {
        std::swap(block_, other.block_);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    size_t UseCount() const {
        if (block_ == nullptr) {
            return 0;
        }
        return block_->StrongUseCount();
    }
    bool Expired() const {
        return UseCount() == 0;
    }
    SharedPtr<T> Lock() const {
        if (Expired()) {
            return SharedPtr<T>();
        } else {
            return SharedPtr<T>(*this);
        }
    }
    BaseControlBlock* GetBlock() const {
        return block_;
    }

private:
    void Increment() {
        if (block_) {
            block_->WeakIncrement();
        }
    }
    void Decrement() {
        if (block_) {
            block_->WeakDecrement();
            if (block_->CanBeDeleted()) {
                delete block_;
            }
        }
    }
    BaseControlBlock* block_ = nullptr;
};

#endif
