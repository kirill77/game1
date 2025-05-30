#pragma once

#include "IDevice.h"

struct D3D12Device : public IDevice
{
    D3D12Device(const std::string& vendor);

private:
};