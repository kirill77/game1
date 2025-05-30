#pragma once

#include "IQueue.hpp"
#include <memory>

struct IResource;

// contains Window + swap chain
struct IWindow : public std::enable_shared_from_this<IWindow>
{
    virtual ~IWindow() = default;
    inline std::shared_ptr<IQueue> getQueue() { return m_pQueue; }
    virtual std::shared_ptr<IResource> getNextImage() = 0;
    virtual void present() = 0;
    virtual bool pollEvents() = 0;

protected:
    std::shared_ptr<IDevice> m_pDevice;
    std::shared_ptr<IQueue> m_pQueue;
};
