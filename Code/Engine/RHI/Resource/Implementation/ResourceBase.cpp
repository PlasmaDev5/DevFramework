#include <RHI/Resource/ResourceBase.h>

plRHIResourceBase::plRHIResourceBase()
    : m_ResourceStateTracker(*this)
{
}

plRHIResourceType plRHIResourceBase::GetResourceType() const
{
    return ResourceType;
}

plRHIResourceFormat::Enum plRHIResourceBase::GetFormat() const
{
    return Format;
}

plRHIMemoryType plRHIResourceBase::GetMemoryType() const
{
    return m_MemoryType;
}

void plRHIResourceBase::UpdateUploadBuffer(plUInt64 bufferOffset, const void* data, plUInt64 numBytes)
{
    void* dstData = Map() + bufferOffset;
    memcpy(dstData, data, numBytes);
    Unmap();
}

void plRHIResourceBase::UpdateUploadBufferWithTextureData(plUInt64 bufferOffset, plUInt32 bufferRowPitch, plUInt32 bufferDepthPitch,
    const void* srcData, plUInt32 srcRowPitch, plUInt32 srcDepthPitch, plUInt32 numRows, plUInt32 numSlices)
{
    void* dstData = Map() + bufferOffset;
    for (plUInt32 z = 0; z < numSlices; ++z)
    {
        plUInt8* destSlice = reinterpret_cast<plUInt8*>(dstData) + bufferDepthPitch * z;
        const plUInt8* srcSlice = reinterpret_cast<const plUInt8*>(srcData) + srcDepthPitch * z;
        for (plUInt32 y = 0; y < numRows; ++y)
        {
            memcpy(destSlice + bufferRowPitch * y, srcSlice + srcRowPitch * y, srcRowPitch);
        }
    }
    Unmap();
}

plRHIResourceState plRHIResourceBase::GetInitialState() const
{
    return m_InitialState;
}

bool plRHIResourceBase::IsBackBuffer() const
{
  return m_IsBackBuffer;
}

void plRHIResourceBase::SetInitialState(plRHIResourceState state)
{
  m_InitialState = state;
  m_ResourceStateTracker.SetResourceState(m_InitialState);
}

plRHIResourceStateTracker& plRHIResourceBase::GetGlobalResourceStateTracker()
{
  return m_ResourceStateTracker;
}

const plRHIResourceStateTracker& plRHIResourceBase::GetGlobalResourceStateTracker() const
{
  return m_ResourceStateTracker;
}
