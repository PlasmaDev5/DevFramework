#include <RHIDX12/View/DXView.h>
#include <RHIDX12/Device/DXDevice.h>
#include <RHIDX12/Utilities/DXUtility.h>
#include <cassert>
#include <directx/d3d12.h>

plDXView::plDXView(plDXDevice& device, const plSharedPtr<plDXResource>& resource, const plRHIViewDesc& m_ViewDesc)
    : m_Device(device)
    , m_Resource(resource)
    , m_ViewDesc(m_ViewDesc)
{
    if (m_ViewDesc.viewType == plRHIViewType::kShadingRateSource)
    {
        return;
    }

    m_Handle = m_Device.GetCPUDescriptorPool().AllocateDescriptor(m_ViewDesc.viewType);

    if (m_Resource)
    {
        CreateView();
    }

    if (m_ViewDesc.bindless)
    {
        assert(m_ViewDesc.viewType != plRHIViewType::kUnknown);
        assert(m_ViewDesc.viewType != plRHIViewType::kRenderTarget);
        assert(m_ViewDesc.viewType != plRHIViewType::kDepthStencil);
        if (m_ViewDesc.viewType == plRHIViewType::kSampler)
        {
            m_Range = std::make_shared<plDXGPUDescriptorPoolRange>(m_Device.GetGPUDescriptorPool().Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 1));
        }
        else
        {
            m_Range = std::make_shared<plDXGPUDescriptorPoolRange>(m_Device.GetGPUDescriptorPool().Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1));
        }
        m_Range->CopyCpuHandle(0, m_Handle->GetCpuHandle());
    }
}

D3D12_CPU_DESCRIPTOR_HANDLE plDXView::GetHandle()
{
    return m_Handle->GetCpuHandle();
}

void plDXView::CreateView()
{
    switch (m_ViewDesc.viewType)
    {
    case plRHIViewType::kTexture:
    case plRHIViewType::kBuffer:
    case plRHIViewType::kStructuredBuffer:
        CreateSRV();
        break;
    case plRHIViewType::kAccelerationStructure:
        CreateRAS();
        break;
    case plRHIViewType::kRWTexture:
    case plRHIViewType::kRWBuffer:
    case plRHIViewType::kRWStructuredBuffer:
        CreateUAV();
        break;
    case plRHIViewType::kConstantBuffer:
        CreateCBV();
        break;
    case plRHIViewType::kSampler:
        CreateSampler();
        break;
    case plRHIViewType::kRenderTarget:
        CreateRTV();
        break;
    case plRHIViewType::kDepthStencil:
        CreateDSV();
        break;
    default:
        assert(false);
        break;
    }
}

void plDXView::CreateSRV()
{
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = m_Resource->desc.Format;

    if (plDXUtils::IsTypelessDepthStencil(srvDesc.Format))
    {
        if (m_ViewDesc.planeSlice == 0)
        {
            srvDesc.Format = plDXUtils::DepthReadFromTypeless(srvDesc.Format);
        }
        else
        {
          srvDesc.Format = plDXUtils::StencilReadFromTypeless(srvDesc.Format);
        }
    }

    auto setupMips = [&](plUInt32& mostDetailedMip, plUInt32& mipLevels)
    {
        mostDetailedMip = GetBaseMipLevel();
        mipLevels = GetLevelCount();
    };

    switch (m_ViewDesc.dimension)
    {
    case plRHIViewDimension::kTexture1D:
    {
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
        setupMips(srvDesc.Texture1D.MostDetailedMip, srvDesc.Texture1D.MipLevels);
        break;
    }
    case plRHIViewDimension::kTexture1DArray:
    {
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
        srvDesc.Texture1DArray.FirstArraySlice = GetBaseArrayLayer();
        srvDesc.Texture1DArray.ArraySize = GetLayerCount();
        setupMips(srvDesc.Texture1DArray.MostDetailedMip, srvDesc.Texture1DArray.MipLevels);
        break;
    }
    case plRHIViewDimension::kTexture2D:
    {
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.PlaneSlice = m_ViewDesc.planeSlice;
        setupMips(srvDesc.Texture2D.MostDetailedMip, srvDesc.Texture2D.MipLevels);
        break;
    }
    case plRHIViewDimension::kTexture2DArray:
    {
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
        srvDesc.Texture2DArray.PlaneSlice = m_ViewDesc.planeSlice;
        srvDesc.Texture2DArray.FirstArraySlice = GetBaseArrayLayer();
        srvDesc.Texture2DArray.ArraySize = GetLayerCount();
        setupMips(srvDesc.Texture2DArray.MostDetailedMip, srvDesc.Texture2DArray.MipLevels);
        break;
    }
    case plRHIViewDimension::kTexture2DMS:
    {
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
        break;
    }
    case plRHIViewDimension::kTexture2DMSArray:
    {
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
        srvDesc.Texture2DMSArray.FirstArraySlice = GetBaseArrayLayer();
        srvDesc.Texture2DMSArray.ArraySize = GetLayerCount();
        break;
    }
    case plRHIViewDimension::kTexture3D:
    {
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
        setupMips(srvDesc.Texture3D.MostDetailedMip, srvDesc.Texture3D.MipLevels);
        break;
    }
    case plRHIViewDimension::kTextureCube:
    {
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
        setupMips(srvDesc.TextureCube.MostDetailedMip, srvDesc.TextureCube.MipLevels);
        break;
    }
    case plRHIViewDimension::kTextureCubeArray:
    {
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
        srvDesc.TextureCubeArray.First2DArrayFace = GetBaseArrayLayer() / 6;
        srvDesc.TextureCubeArray.NumCubes = GetLayerCount() / 6;
        setupMips(srvDesc.TextureCubeArray.MostDetailedMip, srvDesc.TextureCubeArray.MipLevels);
        break;
    }
    case plRHIViewDimension::kBuffer:
    {
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        plUInt32 stride = 0;
        if (m_ViewDesc.viewType == plRHIViewType::kBuffer)
        {
          srvDesc.Format = plDXUtils::ToDXGIFormat(m_ViewDesc.bufferFormat);
          stride = plRHIResourceFormat::GetFormatStride(m_ViewDesc.bufferFormat);
        }
        else
        {
          assert(m_ViewDesc.viewType == plRHIViewType::kStructuredBuffer);
          srvDesc.Buffer.StructureByteStride = m_ViewDesc.structureStride;
          stride = srvDesc.Buffer.StructureByteStride;
        }
        plUInt64 size = plMath::Min(m_Resource->desc.Width, m_ViewDesc.bufferSize);
        srvDesc.Buffer.FirstElement = m_ViewDesc.offset / stride;
        srvDesc.Buffer.NumElements = (plUInt32)((size - m_ViewDesc.offset) / (stride));
        break;
    }
    default:
    {
        assert(false);
        break;
    }
    }

    m_Device.GetDevice()->CreateShaderResourceView(m_Resource->resource.Get(), &srvDesc, m_Handle->GetCpuHandle());
}

void plDXView::CreateRAS()
{
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
    srvDesc.RaytracingAccelerationStructure.Location = m_Resource->accelerationStructureHandle;
    m_Device.GetDevice()->CreateShaderResourceView(nullptr, &srvDesc, m_Handle->GetCpuHandle());
}

void plDXView::CreateUAV()
{
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = m_Resource->desc.Format;

    switch (m_ViewDesc.dimension)
    {
      case plRHIViewDimension::kTexture1D:
      {
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
        uavDesc.Texture1D.MipSlice = GetBaseMipLevel();
        break;
      }
      case plRHIViewDimension::kTexture1DArray:
      {
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
        uavDesc.Texture1DArray.FirstArraySlice = GetBaseArrayLayer();
        uavDesc.Texture1DArray.ArraySize = GetLayerCount();
        uavDesc.Texture1DArray.MipSlice = GetBaseMipLevel();
        break;
      }
      case plRHIViewDimension::kTexture2D:
      {
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        uavDesc.Texture2D.PlaneSlice = m_ViewDesc.planeSlice;
        uavDesc.Texture2D.MipSlice = GetBaseMipLevel();
        break;
      }
      case plRHIViewDimension::kTexture2DArray:
      {
          uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
          uavDesc.Texture2DArray.PlaneSlice = m_ViewDesc.planeSlice;
          uavDesc.Texture2DArray.FirstArraySlice = GetBaseArrayLayer();
          uavDesc.Texture2DArray.ArraySize = GetLayerCount();
          uavDesc.Texture2DArray.MipSlice = GetBaseMipLevel();
          break;
      }
      case plRHIViewDimension::kTexture3D:
      {
          uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
          uavDesc.Texture3D.MipSlice = GetBaseMipLevel();
          break;
      }
      case plRHIViewDimension::kBuffer:
      {
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        plUInt32 stride = 0;
        if (m_ViewDesc.viewType == plRHIViewType::kRWBuffer)
        {
          uavDesc.Format = plDXUtils::ToDXGIFormat(m_ViewDesc.bufferFormat);
          stride = plRHIResourceFormat::GetFormatStride(m_ViewDesc.bufferFormat);
        }
        else
        {
          assert(m_ViewDesc.viewType == plRHIViewType::kRWStructuredBuffer);
          uavDesc.Buffer.StructureByteStride = m_ViewDesc.structureStride;
          stride = uavDesc.Buffer.StructureByteStride;
        }

        plUInt64 size = plMath::Min(m_Resource->desc.Width, m_ViewDesc.bufferSize);
        uavDesc.Buffer.FirstElement = m_ViewDesc.offset / stride;
        uavDesc.Buffer.NumElements = (plUInt32)((size - m_ViewDesc.offset) / (stride));

        break;
      }
      default:
      {
          assert(false);
          break;
      }
    }

    m_Device.GetDevice()->CreateUnorderedAccessView(m_Resource->resource.Get(), nullptr, &uavDesc, m_Handle->GetCpuHandle());
}

void plDXView::CreateRTV()
{
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = m_Resource->desc.Format;

    switch (m_ViewDesc.dimension)
    {
      case plRHIViewDimension::kTexture1D:
      {
          rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1D;
          rtvDesc.Texture1D.MipSlice = GetBaseMipLevel();
          break;
      }
      case plRHIViewDimension::kTexture1DArray:
      {
          rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
          rtvDesc.Texture1DArray.FirstArraySlice = GetBaseArrayLayer();
          rtvDesc.Texture1DArray.ArraySize = GetLayerCount();
          rtvDesc.Texture1DArray.MipSlice = GetBaseMipLevel();
          break;
      }
      case plRHIViewDimension::kTexture2D:
      {
          rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
          rtvDesc.Texture2D.PlaneSlice = m_ViewDesc.planeSlice;
          rtvDesc.Texture2D.MipSlice = GetBaseMipLevel();
          break;
      }
      case plRHIViewDimension::kTexture2DArray:
      {
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
        rtvDesc.Texture2DArray.PlaneSlice = m_ViewDesc.planeSlice;
        rtvDesc.Texture2DArray.FirstArraySlice = GetBaseArrayLayer();
        rtvDesc.Texture2DArray.ArraySize = GetLayerCount();
        rtvDesc.Texture2DArray.MipSlice = GetBaseMipLevel();
        break;
      }
      case plRHIViewDimension::kTexture2DMS:
      {
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
        break;
      }
      case plRHIViewDimension::kTexture2DMSArray:
      {
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
        rtvDesc.Texture2DMSArray.FirstArraySlice = GetBaseArrayLayer();
        rtvDesc.Texture2DMSArray.ArraySize = GetLayerCount();
        break;
      }
      case plRHIViewDimension::kTexture3D:
      {
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
        rtvDesc.Texture3D.MipSlice = GetBaseMipLevel();
        break;
      }
      default:
      {
        assert(false);
        break;
      }
    }

    m_Device.GetDevice()->CreateRenderTargetView(m_Resource->resource.Get(), &rtvDesc, m_Handle->GetCpuHandle());
}

void plDXView::CreateDSV()
{
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = plDXUtils::DepthStencilFromTypeless(m_Resource->desc.Format);

    switch (m_ViewDesc.dimension)
    {
      case plRHIViewDimension::kTexture1D:
      {
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1D;
        dsvDesc.Texture1D.MipSlice = GetBaseMipLevel();
        break;
      }
      case plRHIViewDimension::kTexture1DArray:
      {
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
        dsvDesc.Texture1DArray.FirstArraySlice = GetBaseArrayLayer();
        dsvDesc.Texture1DArray.ArraySize = GetLayerCount();
        dsvDesc.Texture1DArray.MipSlice = GetBaseMipLevel();
        break;
      }
      case plRHIViewDimension::kTexture2D:
      {
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Texture2D.MipSlice = GetBaseMipLevel();
        break;
      }
      case plRHIViewDimension::kTexture2DArray:
      {
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
        dsvDesc.Texture2DArray.FirstArraySlice = GetBaseArrayLayer();
        dsvDesc.Texture2DArray.ArraySize = GetLayerCount();
        dsvDesc.Texture2DArray.MipSlice = GetBaseMipLevel();
        break;
      }
      case plRHIViewDimension::kTexture2DMS:
      {
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
        break;
      }
      case plRHIViewDimension::kTexture2DMSArray:
      {
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
        dsvDesc.Texture2DMSArray.FirstArraySlice = GetLayerCount();
        dsvDesc.Texture2DMSArray.ArraySize = GetLayerCount();
        break;
      }
      default:
      {
        assert(false);
        break;
      }
    }

    m_Device.GetDevice()->CreateDepthStencilView(m_Resource->resource.Get(), &dsvDesc, m_Handle->GetCpuHandle());
}

void plDXView::CreateCBV()
{
    D3D12_CONSTANT_BUFFER_VIEW_DESC cvbDesc = {};
    cvbDesc.BufferLocation = m_Resource->resource->GetGPUVirtualAddress();
    cvbDesc.SizeInBytes = (plUInt32)plMath::Min(m_Resource->desc.Width, m_ViewDesc.bufferSize);
    assert(cvbDesc.SizeInBytes % 256 == 0);
    m_Device.GetDevice()->CreateConstantBufferView(&cvbDesc, m_Handle->GetCpuHandle());
}

void plDXView::CreateSampler()
{
    m_Device.GetDevice()->CreateSampler(&m_Resource->samplerDesc, m_Handle->GetCpuHandle());
}

plSharedPtr<plRHIResource> plDXView::GetResource()
{
    return m_Resource;
}

plUInt32 plDXView::GetDescriptorId() const
{
    if (m_Range)
        return m_Range->GetOffset();
    return 0;
}

plUInt32 plDXView::GetBaseMipLevel() const
{
    return m_ViewDesc.baseMipLevel;
}

plUInt32 plDXView::GetLevelCount() const
{
  return std::min<plUInt32>(m_ViewDesc.levelCount, m_Resource->GetLevelCount() - m_ViewDesc.baseMipLevel);
}

plUInt32 plDXView::GetBaseArrayLayer() const
{
    return m_ViewDesc.baseArrayLayer;
}

plUInt32 plDXView::GetLayerCount() const
{
    return std::min<plUInt32>(m_ViewDesc.layerCount, m_Resource->GetLayerCount() - m_ViewDesc.baseArrayLayer);
}
