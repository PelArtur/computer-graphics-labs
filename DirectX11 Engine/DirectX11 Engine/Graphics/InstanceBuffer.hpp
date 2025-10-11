#pragma once
#include <d3d11.h>
#include <wrl/client.h>


template<class T>
class InstanceBuffer
{
private:
    Microsoft::WRL::ComPtr<ID3D11Buffer> buffer;
    UINT stride = sizeof(T);
    UINT instanceCount = 0;

public:
    InstanceBuffer() {}

    InstanceBuffer(const InstanceBuffer<T>& rhs)
    {
        this->buffer = rhs.buffer;
        this->instanceCount = rhs.instanceCount;
        this->stride = rhs.stride;
    }

    InstanceBuffer<T>& operator=(const InstanceBuffer<T>& a)
    {
        this->buffer = a.buffer;
        this->instanceCount = a.instanceCount;
        this->stride = a.stride;
        return *this;
    }

    ID3D11Buffer* Get() const { return buffer.Get(); }
    ID3D11Buffer* const* GetAddressOf() const { return buffer.GetAddressOf(); }
    const UINT Stride() const { return this->stride; }
    const UINT* StridePtr() const { return &this->stride; }
    UINT InstanceCount() const { return this->instanceCount; }

    HRESULT Initialize(ID3D11Device* device, T* data, UINT instanceCount)
    {
        if (buffer.Get() != nullptr)
            buffer.Reset();

        this->instanceCount = instanceCount;

        D3D11_BUFFER_DESC indexBufferDesc = {};
        indexBufferDesc.Usage = D3D11_USAGE_DYNAMIC; // dynamic, may update per frame
        indexBufferDesc.ByteWidth = stride * instanceCount;
        indexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        indexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        indexBufferDesc.MiscFlags = 0;

        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem = data;

        HRESULT hr = device->CreateBuffer(&indexBufferDesc, &initData, this->buffer.GetAddressOf());
        return hr;
    }

    bool Update(ID3D11DeviceContext* deviceContext, T* data, UINT instanceCount)
    {
        if (!buffer) 
            return false;

        D3D11_MAPPED_SUBRESOURCE mappedResource;
        HRESULT hr = deviceContext->Map(buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
        if (FAILED(hr)) 
            return false;

        memcpy(mappedResource.pData, data, sizeof(T) * instanceCount);
        deviceContext->Unmap(buffer.Get(), 0);
        this->instanceCount = instanceCount;
        return true;
    }
};
