#include <RHIDX12/CommandList/DXCommandList.h>
#include <RHIDX12/Device/DXDevice.h>
#include <RHIDX12/Fence/DXFence.h>
#include <RHIDX12/CommandQueue/DXCommandQueue.h>
#include <RHIDX12/Resource/DXResource.h>
#include <RHIDX12/Utilities/DXUtility.h>

plDXCommandQueue::plDXCommandQueue(plDXDevice& device, plRHICommandListType type)
  : m_Device(device)
{
  D3D12_COMMAND_LIST_TYPE dxType{};
  switch (type)
  {
    case plRHICommandListType::kGraphics:
      dxType = D3D12_COMMAND_LIST_TYPE_DIRECT;
      break;
    case plRHICommandListType::kCompute:
      dxType = D3D12_COMMAND_LIST_TYPE_COMPUTE;
      break;
    case plRHICommandListType::kCopy:
      dxType = D3D12_COMMAND_LIST_TYPE_COPY;
      break;
    default:
      assert(false);
      break;
  }
  D3D12_COMMAND_QUEUE_DESC queueDesc = {};
  queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_HIGH;
  queueDesc.Type = dxType;
  PL_ASSERT_ALWAYS(m_Device.GetDevice()->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_CommandQueue)) == S_OK, "");
}

void plDXCommandQueue::Wait(const plSharedPtr<plRHIFence>& fence, plUInt64 value)
{
  plSharedPtr<plDXFence> dxFence = fence.Downcast<plDXFence>();
  PL_ASSERT_ALWAYS(m_CommandQueue->Wait(dxFence->GetFence().Get(), value) == S_OK, "");
}

void plDXCommandQueue::Signal(const plSharedPtr<plRHIFence>& fence, plUInt64 value)
{
  plSharedPtr<plDXFence> dxFence = fence.Downcast<plDXFence>();
  PL_ASSERT_ALWAYS(m_CommandQueue->Signal(dxFence->GetFence().Get(), value) == S_OK, "");
}

void plDXCommandQueue::ExecuteCommandLists(const std::vector<plSharedPtr<plRHICommandList>>& commandLists)
{
  plDynamicArray<ID3D12CommandList*> dxCommandLists;
  for (auto& commandList : commandLists)
  {
    if (!commandList)
      continue;
    plSharedPtr<plDXCommandList> dxCommandList = commandList.Downcast<plDXCommandList>();
    dxCommandLists.PushBack(dxCommandList->GetCommandList().Get());
  }
  if (!dxCommandLists.IsEmpty())
    m_CommandQueue->ExecuteCommandLists(dxCommandLists.GetCount(), dxCommandLists.GetData());
}

plDXDevice& plDXCommandQueue::GetDevice()
{
  return m_Device;
}

ComPtr<ID3D12CommandQueue> plDXCommandQueue::GetQueue()
{
  return m_CommandQueue;
}
