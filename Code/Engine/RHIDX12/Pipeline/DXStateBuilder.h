#pragma once
#include <RHIDX12/RHIDX12DLL.h>
#include <vector>
#include <directx/d3d12.h>

class plDXStateBuilder
{
public:
    template<typename T, typename U>
    void AddState(const U& state)
    {
        plUInt32 offset = m_Data.GetCount();
        m_Data.SetCountUninitialized(offset + (plUInt32)sizeof(T));
        reinterpret_cast<T&>(m_Data[offset]) = state;
    }

    D3D12_PIPELINE_STATE_STREAM_DESC GetDesc()
    {
        D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = {};
        streamDesc.pPipelineStateSubobjectStream = m_Data.GetData();
        streamDesc.SizeInBytes = m_Data.GetCount();
        return streamDesc;
    } 

private:
    plDynamicArray<plUInt8> m_Data;
};
