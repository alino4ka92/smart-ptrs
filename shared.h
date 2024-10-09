#pragma once

#include "sw_fwd.h"  // Forward declaration
#include "weak.h"

#include <cstddef>  // std::nullptr_t
#include <utility>


class BaseEnableSharedFromThis {
};
template <class T>
class EnableSharedFromThis;

// https://en.cppreference.com/w/cpp/memory/shared_ptr
template <typename T>
class SharedPtr {
public:
    template <class Y>
    friend class SharedPtr;
    
    template <class Y>
    friend class EnableSharedFromThis;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    template<class Y> 
    void InitWeakThis(EnableSharedFromThis<Y>* e) {
        e->weak_this_ = *this;
    }

    SharedPtr() = default;
    SharedPtr(std::nullptr_t) : SharedPtr() {
    }

    SharedPtr(BaseControlBlock* block, T* ptr) : block_(block), ptr_(ptr) {
        if constexpr (std::is_convertible_v<T*, BaseEnableSharedFromThis*>) {
            InitWeakThis(ptr);
        }
    }

    template <class Y>
    explicit SharedPtr(Y* ptr) : block_(new PointingControlBlock<Y>(ptr)), ptr_(ptr) {
        if constexpr (std::is_convertible_v<T*, BaseEnableSharedFromThis*>) {
            InitWeakThis(ptr);
        }
    }

    explicit SharedPtr(T* ptr) : block_(new PointingControlBlock<T>(ptr)), ptr_(ptr) {
        if constexpr (std::is_convertible_v<T*, BaseEnableSharedFromThis*>) {
            InitWeakThis(ptr);
        }
    }

    template <class Y>
    SharedPtr(const SharedPtr<Y>& other) : block_(other.block_), ptr_(other.Get()) {
        if (block_) {
            block_->StrongIncrement();
        }
    }

    SharedPtr(const SharedPtr& other) : block_(other.block_), ptr_(other.Get()) {
        if (block_) {
            block_->StrongIncrement();
        }
    }

    template <class Y>
    SharedPtr(SharedPtr<Y>&& other) : block_(other.block_), ptr_(other.Get()) {
        other.block_ = nullptr;
        other.ptr_ = nullptr;
    }

    SharedPtr(SharedPtr&& other) : block_(other.block_), ptr_(other.Get()) {
        other.block_ = nullptr;
        other.ptr_ = nullptr;
    }

    // Aliasing constructor
    // #8 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    template <typename Y>
    SharedPtr(const SharedPtr<Y>& other, T* ptr) : block_(other.block_), ptr_(ptr) {
        block_->StrongIncrement();
    }

    // Promote `WeakPtr`
    // #11 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    explicit SharedPtr(const WeakPtr<T>& other) {
        if (other.Expired()) {
            throw BadWeakPtr();
        }
        block_ = other.GetBlock();
        ptr_ = static_cast<T*>(block_->Get());
        block_->StrongIncrement();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    template <class Y>
    SharedPtr& operator=(const SharedPtr<Y>& other) {
        Reset();
        block_ = other.block_;
        if (block_) {
            block_->StrongIncrement();
        }
        ptr_ = other.ptr_;

        return *this;
    }

    SharedPtr& operator=(const SharedPtr& other) {
        Reset();
        block_ = other.block_;
        if (block_) {
            block_->StrongIncrement();
        }
        ptr_ = other.ptr_;

        return *this;
    }

    template <class Y>
    SharedPtr& operator=(SharedPtr<Y>&& other) {
        Reset();
        block_ = other.block_;
        ptr_ = other.ptr_;
        other.block_ = nullptr;
        other.ptr_ = nullptr;

        return *this;
    }

    SharedPtr& operator=(SharedPtr&& other) {
        Reset();
        block_ = other.block_;
        ptr_ = other.ptr_;
        other.block_ = nullptr;
        other.ptr_ = nullptr;

        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~SharedPtr() {
        Reset();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void Reset() {
        if (block_) {
            block_->StrongDecrement();
            if (block_->CanBeDeleted()) {
                delete block_;
            }
        } 
        block_ = nullptr;
        ptr_ = nullptr;
    }

    template <class Y>
    void Reset(Y* ptr) {
        Reset();
        block_ = new PointingControlBlock<Y>(ptr);
        ptr_ = ptr;
    }

    void Swap(SharedPtr& other) {
        std::swap(block_, other.block_);
        std::swap(ptr_, other.ptr_);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    T* Get() const {
        return ptr_;
    }
    T& operator*() const {
        return *ptr_;
    }
    T* operator->() const {
        return ptr_;
    }
    size_t UseCount() const {
        if (block_ == nullptr) {
            return 0;
        }
        return block_->StrongUseCount();
    }
    explicit operator bool() const {
        return ptr_ != nullptr;
    }
    BaseControlBlock* GetBlock() const {
        return block_;
    }

private:
    BaseControlBlock* block_ = nullptr;
    T* ptr_ = nullptr;
};

template <typename T, typename U>
inline bool operator==(const SharedPtr<T>& left, const SharedPtr<U>& right) {
    return left.GetBlock() == right.GetBlock();
}

// Allocate memory only once
template <typename T, typename... Args>
SharedPtr<T> MakeShared(Args&&... args) {
    EmplacingControlBlock<T>* block = new EmplacingControlBlock<T>(std::forward<Args>(args)...);
    return SharedPtr<T>(block, static_cast<T*>(block->Get()));
}

// Look for usage examples in tests

template <typename T>
class EnableSharedFromThis : public BaseEnableSharedFromThis {
public:
    template <class Y>
    friend class SharedPtr;

    SharedPtr<T> SharedFromThis() {
        return SharedPtr<T>(weak_this_);
    }
    SharedPtr<const T> SharedFromThis() const {
        return SharedPtr<const T>(weak_this_);
    }

    WeakPtr<T> WeakFromThis() noexcept {
        return weak_this_;
    }
    WeakPtr<const T> WeakFromThis() const noexcept {
        return weak_this_;
    }
private:
    WeakPtr<T> weak_this_ = WeakPtr<T>();
};
