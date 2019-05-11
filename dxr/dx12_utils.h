#pragma once

#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl.h>
#include <iostream>

// Utilities for general DX12 ease of use

#define CHECK_ERR(FN) \
	{ \
		auto fn_err = FN; \
		if (FAILED(fn_err)) { \
			std::cout << #FN << " failed due to " \
				<< std::hex << fn_err << std::endl << std::flush; \
			throw std::runtime_error(#FN); \
		}\
	}

extern const D3D12_HEAP_PROPERTIES UPLOAD_HEAP_PROPS;
extern const D3D12_HEAP_PROPERTIES DEFAULT_HEAP_PROPS;
extern const D3D12_HEAP_PROPERTIES READBACK_HEAP_PROPS;

// Convenience for making resource transition barriers
D3D12_RESOURCE_BARRIER barrier_transition(ID3D12Resource *res, D3D12_RESOURCE_STATES before,
		D3D12_RESOURCE_STATES after);
D3D12_RESOURCE_BARRIER barrier_transition(Microsoft::WRL::ComPtr<ID3D12Resource> &res,
		D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);

// Convenience for making UAV transition barriers
D3D12_RESOURCE_BARRIER barrier_uav(ID3D12Resource *res);
D3D12_RESOURCE_BARRIER barrier_uav(Microsoft::WRL::ComPtr<ID3D12Resource> &res);

class Resource {
protected:
	Microsoft::WRL::ComPtr<ID3D12Resource> res;
	D3D12_HEAP_TYPE rheap;
	D3D12_RESOURCE_STATES rstate;
	
	friend D3D12_RESOURCE_BARRIER barrier_transition(Resource &res, D3D12_RESOURCE_STATES after);

public:
	virtual ~Resource();

	ID3D12Resource* operator->();
	ID3D12Resource* get();
	D3D12_HEAP_TYPE heap() const;
	D3D12_RESOURCE_STATES state() const;
};

D3D12_RESOURCE_BARRIER barrier_transition(Resource &res, D3D12_RESOURCE_STATES after);
D3D12_RESOURCE_BARRIER barrier_uav(Resource &res);

class Buffer : public Resource {
	size_t buf_size;

	static D3D12_RESOURCE_DESC res_desc(size_t nbytes, D3D12_RESOURCE_FLAGS flags);

	static Buffer create(ID3D12Device *device, size_t nbytes,
		D3D12_RESOURCE_STATES state, D3D12_HEAP_PROPERTIES props, D3D12_RESOURCE_DESC desc);

public:
	Buffer();

	// Allocate an upload buffer of the desired size
	static Buffer upload(ID3D12Device *device, size_t nbytes,
		D3D12_RESOURCE_STATES state,
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);
	// Allocate a GPU-side buffer of the desired size
	static Buffer default(ID3D12Device *device, size_t nbytes,
		D3D12_RESOURCE_STATES state,
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);
	// Allocate a readback buffer of the desired size
	static Buffer readback(ID3D12Device *device, size_t nbytes,
		D3D12_RESOURCE_STATES state,
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

	// Map the whole range for potentially being read
	void* map();
	// Map to read a specific or empty range
	void* map(D3D12_RANGE read);
	
	// Unmap and mark the whole range as written
	void unmap();
	// Unmap and mark a specific range as written
	void unmap(D3D12_RANGE written);

	size_t size() const;
};