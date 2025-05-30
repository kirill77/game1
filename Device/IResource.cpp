#include "framework.h"
#include "IResource.h"

uint32_t IResource::getBytesPerPixel(eFormat format)
{
    switch (format)
    {
    case eFormatRGBA8:
        return 4;  // 8 bits per channel * 4 channels = 32 bits = 4 bytes
    case eFormatUnknown:
    default:
        return 0;  // Unknown format, return 0
    }
} 