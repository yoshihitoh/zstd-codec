#pragma once

#include <functional>


template <typename T>
class Resource
{
public:
    using CloseHandler = std::function<void(T*)>;

    Resource(T* resource, CloseHandler close_handler)
        : resource_(resource)
        , close_handler_(close_handler)
    {
    }

    ~Resource()
    {
        Close();
    }

    T* get() const { return resource_; }

    void Close()
    {
        if (resource_ != nullptr) {
            close_handler_(resource_);
            resource_ = nullptr;
        }
    }

private:
    T*  resource_;
    CloseHandler close_handler_;
};
