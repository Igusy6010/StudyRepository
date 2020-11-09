#include <Windows.h>
#include <tchar.h>
#include <vector>
#include <string>

#ifdef _DEBUG
#include <iostream>
#endif

#include <d3d12.h>
#include <dxgi1_6.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

using namespace std;

// �E�B���h�E�T�C�Y
const int window_width = 1280;
const int window_height = 768;

// ��{�I�u�W�F�N�g
ID3D12Device* _dev = nullptr;
IDXGIFactory6* _dxgiFactory = nullptr;
IDXGISwapChain4* _swapchain = nullptr;

// �R�}���h���X�g���쐬����
ID3D12CommandAllocator* _cmdAllocator = nullptr;
ID3D12GraphicsCommandList* _cmdList = nullptr;
ID3D12CommandQueue* _cmdQueue = nullptr;

// �f�B�X�N���v�^�q�[�v
ID3D12DescriptorHeap* _rtvHeaps = nullptr;

// �o�b�N�o�b�t�@�[
vector<ID3D12Resource*> _backBuffers;


/// <summary>
/// DirectX12 �������� 
/// </summary>
void InitializeDirectX12(HWND hwnd)
{
#ifdef _DEBUG
	auto result = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&_dxgiFactory));
#else
	auto result = CreateDXGIFactory1(IID_PPV_ARGS(&_dxgiFactory));
#endif

	// �A�_�v�^�[�̗񋓗p
	vector<IDXGIAdapter*> adapters;

	// �����ɓ���̖��O�����A�_�v�^�[�I�u�W�F�N�g������
	IDXGIAdapter* tmpAdapter = nullptr;
	for (int i = 0; _dxgiFactory->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		adapters.push_back(tmpAdapter);
	}

	for (auto adpt : adapters)
	{
		DXGI_ADAPTER_DESC adesc = {};
		adpt->GetDesc(&adesc);	// �A�_�v�^�[�̐����I�u�W�F�N�g�擾

		wstring strDesc = adesc.Description;

		// �T�������A�_�v�^�[�̖��O���m�F
		if (strDesc.find(L"NVIDIA") != std::string::npos)
		{
			tmpAdapter = adpt;
			break;
		}
	}


	// �t���[�`���[���x�����ŐV���猩�Ă���
	D3D_FEATURE_LEVEL levels[] = {
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};

	// Direct3D �f�o�C�X�̏�����
	D3D_FEATURE_LEVEL featureLevel;
	for (auto lv : levels)
	{
		if (D3D12CreateDevice(tmpAdapter, lv, IID_PPV_ARGS(&_dev)) == S_OK)
		{
			featureLevel = lv;
			break;	// �����\�ȃo�[�W�����������������[�v��ł��؂�
		}
	}

	// �A�_�v�^�[�̏�����
	for (auto adapter : adapters)
	{
		adapter->Release();
	}
	vector<IDXGIAdapter*>().swap(adapters);

	if (!_dev)
	{
		// CreateDevice �Ɏ��s������A�A�v���P�[�V�������I��������
		PostQuitMessage(0);
	}

	// �R�}���h���X�g���쐬����
	result = _dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_cmdAllocator));
	if (result != S_OK)
	{
		// CreateCommandAllocator �Ɏ��s������A�A�v���P�[�V�������I��������
		PostQuitMessage(0);
	}

	result = _dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _cmdAllocator, nullptr, IID_PPV_ARGS(&_cmdList));
	if (result != S_OK)
	{
		// CreateCommandList �Ɏ��s������A�A�v���P�[�V�������I��������
		PostQuitMessage(0);
	}


	// �R�}���h�L���[�̏���
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};

	// �^�C���A�E�g�Ȃ�
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

	// �A�_�v�^�[��1�����g��Ȃ��Ƃ��� 0 �ł悢
	cmdQueueDesc.NodeMask = 0;

	// �v���C�I���e�B�͓��Ɏw��Ȃ�
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;

	// �R�}���h���X�g�ƍ��킹��
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	// �L���[����
	result = _dev->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&_cmdQueue));
	if (result != S_OK)
	{
		// CreateCommandQueue �Ɏ��s������A�A�v���P�[�V�������I��������
		PostQuitMessage(0);
	}


	//�X���b�v�`�F�[���ݒ�
	DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
	swapchainDesc.Width = window_width;
	swapchainDesc.Height = window_width;
	swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapchainDesc.Stereo = false;
	swapchainDesc.SampleDesc.Count = 1;
	swapchainDesc.SampleDesc.Quality = 0;
	swapchainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
	swapchainDesc.BufferCount = 2;

	// �o�b�N�o�b�t�@�[�͐L�яk�݉\
	swapchainDesc.Scaling = DXGI_SCALING_STRETCH;

	// �t���b�v��͑��₩�ɔj��
	swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	// ���ɂȂ�
	swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;

	// �E�B���h�E<=>�t���X�N���[���؂�ւ��\
	swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	result = _dxgiFactory->CreateSwapChainForHwnd(_cmdQueue, hwnd, &swapchainDesc, nullptr, nullptr, (IDXGISwapChain1**)&_swapchain);
	if (result != S_OK)
	{
		// CreateSwapChainForHwnd �Ɏ��s������A�A�v���P�[�V�������I��������
		PostQuitMessage(0);
	}


	// �f�B�X�N���v�^�q�[�v���쐬
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;	// �����_�[�^�[�Q�b�g�r���[�Ȃ̂�RTV
	heapDesc.NodeMask = 0;
	heapDesc.NumDescriptors = 2;	// �\�E����2��
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;	// ���ɂȂ�

	result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&_rtvHeaps));
	if (result != S_OK)
	{
		// CreateDescriptorHeap �Ɏ��s������A�A�v���P�[�V�������I��������
		PostQuitMessage(0);
	}

	// �X���b�v�`�F�[��
	DXGI_SWAP_CHAIN_DESC swcDesc = {};
	result = _swapchain->GetDesc(&swcDesc);
	if (result != S_OK)
	{
		// CreateDescriptorHeap �Ɏ��s������A�A�v���P�[�V�������I��������
		PostQuitMessage(0);
	}

	// �o�b�N�o�b�t�@�[�쐬
	_backBuffers.resize(swcDesc.BufferCount);
	D3D12_CPU_DESCRIPTOR_HANDLE handle = _rtvHeaps->GetCPUDescriptorHandleForHeapStart();	// �擪�̃A�h���X���擾����
	for (unsigned int idx = 0; idx < swcDesc.BufferCount; ++idx)
	{
		// �X���b�v�`�F�[�����̃o�b�t�@�[�ƃr���[�̊֘A�t��
		result = _swapchain->GetBuffer(idx, IID_PPV_ARGS(&_backBuffers[idx]));
		if (result != S_OK)
		{
			// GetBuffer �Ɏ��s������A�A�v���P�[�V�������I��������
			PostQuitMessage(0);
		}

		// �����_�[�^�[�Q�b�g�r���[�𐶐�����
		_dev->CreateRenderTargetView(_backBuffers[idx], nullptr, handle);

		// �|�C���^�[�����炷
		handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}
}

/// <summary>
/// �R���\�[����ʂɃt�H�[�}�b�g�t���������\��
/// </summary>
/// <param name="format"></param>
/// <param name="">�ϒ�����</param>
/// <remarks>���̊֐��̓f�o�b�O�p�ł��B�f�o�b�O���ɂ������삵�܂���</remarks>
void DebugOutputFormatString(const char* format, ...)
{
#ifdef _DEBUG
	va_list valist;
	va_start(valist, format);
	printf(format, valist);
	va_end(valist);
#endif
}

/// <summary>
/// �ʓ|�����Ǐ����Ȃ���΂����Ȃ��֐� 
/// </summary>
/// <param name="hwnd"></param>
/// <param name="msg"></param>
/// <param name="wparam"></param>
/// <param name="lparam"></param>
/// <returns></returns>
LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	// �E�B���h�E���j�����ꂽ��Ă΂��
	if (msg == WM_DESTROY)
	{
		PostQuitMessage(0); // OS�ɑ΂��āu�������̃A�v���͏I���v�Ɠ`����
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

void EnableDebugLayer()
{
	ID3D12Debug* debugLayer = nullptr;
	auto result = D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer));

	debugLayer->EnableDebugLayer();	// �f�o�b�O���C���[��L���ɂ���
	debugLayer->Release();	// �L����������C���^�[�t�F�C�X���������
}

/// <summary>
/// �G���g���|�C���g
/// </summary>
#ifdef _DEBUG
int main()
#else
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
#endif // _DEBUG
{
//	DebugOutputFormatString("Show window test.");
//	getchar();

	// �E�B���h�E�N���X�̐���&�o�^
	WNDCLASSEX w = {};
	w.cbSize = sizeof(WNDCLASSEX);
	w.lpfnWndProc = (WNDPROC)WindowProcedure;	// �R�[���o�b�N�֐��̎w��
	w.lpszClassName = _T("DX12Sample");			// �A�v���P�[�V�����N���X��(�K���ŗǂ�)
	w.hInstance = GetModuleHandle(nullptr);		// �n���h���̎擾

	// �A�v���P�[�V�����N���X(�E�B���h�E�N���X�̎w���OS�ɓ`����)
	RegisterClassEx(&w);

	// �E�B���h�E�T�C�Y�����߂�
	RECT wrc = {
		0,
		0,
		window_width,
		window_height
	};

	// �֐����g���ăE�B���h�E�̃T�C�Y��␳����
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	// �E�B���h�E�I�u�W�F�N�g�̐���
	HWND hwnd = CreateWindow(
		w.lpszClassName,		// �N���X���̎w��
		_T("DX12�e�X�g"),		// �^�C�g���o�[�̕���
		WS_OVERLAPPEDWINDOW,	// �^�C�g���o�[�Ƌ��E��������E�B���h�E
		CW_USEDEFAULT,			// �\��x���W��OS�ɂ��܂���
		CW_USEDEFAULT,			//  �V y���W��OS�ɂ��܂���
		wrc.right - wrc.left,	// �E�B���h�E��
		wrc.bottom - wrc.top,	// �E�B���h�E����
		nullptr,				// �e�E�B���h�E�n���h��
		nullptr,				// ���j���[�n���h��
		w.hInstance,			// �Ăяo���A�v���P�[�V�����n���h��
		nullptr					// �ǉ��p�����[�^�[
		);

#ifdef _DEBUG
	// �f�o�b�O���C���[���I����
	EnableDebugLayer();
#endif

	// DirectX12�̏�����
	InitializeDirectX12(hwnd);

	// �t�F���X������
	ID3D12Fence* _fence = nullptr;
	UINT64 _fenceVal = 0;
	auto result = _dev->CreateFence(_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));

	// �E�B���h�E�̕\��
	ShowWindow(hwnd, SW_SHOW);

	// 1�t���[���ōs���ׂ���������
	{
		auto bbIdx = _swapchain->GetCurrentBackBufferIndex();

		// ���\�[�X�o���A��ݒ肷��
		D3D12_RESOURCE_BARRIER BarrierDesc = {};
		BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;	// �J��
		BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;	// ���Ɏw��Ȃ�
		BarrierDesc.Transition.pResource = _backBuffers[bbIdx];	// �o�b�N�o�b�t�@�[���\�[�X
		BarrierDesc.Transition.Subresource = 0;

		BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;	// ���O��PRESENT���
		BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;	// �����烌���_�[�^�[�Q�b�g���
		_cmdList->ResourceBarrier(1, &BarrierDesc);	// �o���A�w����s

		// �����_�[�^�[�Q�b�g��ݒ�
		auto rtvH = _rtvHeaps->GetCPUDescriptorHandleForHeapStart();
		rtvH.ptr += bbIdx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		_cmdList->OMSetRenderTargets(1, &rtvH, true, nullptr);

		// ��ʃN���A
		float clearColor[] = { 1.0f, 1.0f, 0.0f, 1.0f }; // ���F
		_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);

		BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		_cmdList->ResourceBarrier(1, &BarrierDesc);	// �o���A�w����s

		// ���߂̃N���[�Y
		_cmdList->Close();

		// �R�}���h���X�g�̎��s
		ID3D12CommandList* cmdLists[] = { _cmdList };
		_cmdQueue->ExecuteCommandLists(1, cmdLists);

		// �R�}���h�̊����҂�
		_cmdQueue->Signal(_fence, ++_fenceVal);

		//while (_fence->GetCompletedValue() != _fenceVal)
		//{
		//	;	// �r�W�[���[�v
		//}
		// �r�W�[���[�v���g��Ȃ����@
		if (_fence->GetCompletedValue() != _fenceVal)
		{
			// �C�x���g�n���h���̎擾
			auto event = CreateEvent(nullptr, false, false, nullptr);

			_fence->SetEventOnCompletion(_fenceVal, event);

			// �C�x���g����������܂ő҂�������(INFINITE)
			WaitForSingleObject(event, INFINITE);

			// �C�x���g�n���h�������
			CloseHandle(event);
		}

		_cmdAllocator->Reset();	// �L���[���N���A
		_cmdList->Reset(_cmdAllocator, nullptr);	// �ĂуR�}���h���X�g�𗭂߂鏀��

		// �t���b�v
		_swapchain->Present(1, 0);
	}

	MSG msg = {};
	while (true)
	{
		if (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// �A�v���P�[�V�������I���Ƃ���message �� WM_QUIT �ɂȂ�
		if (msg.message == WM_QUIT)
		{
			break;
		}
	}

	// �����N���X�͎g��Ȃ��̂œo�^����������
	UnregisterClass(w.lpszClassName, w.hInstance);

	for (auto backBuffer : _backBuffers)
	{
		backBuffer->Release();
	}
	vector<ID3D12Resource*>().swap(_backBuffers);

	if (_fence)
	{
		_fence->Release();
	}

	if (_cmdAllocator)
	{
		_cmdAllocator->Release();
	}
	if (_cmdList)
	{
		_cmdList->Release();
	}
	if (_cmdQueue)
	{
		_cmdQueue->Release();
	}

	if (_rtvHeaps)
	{
		_rtvHeaps->Release();
	}

	_swapchain->Release();
	_dxgiFactory->Release();
	_dev->Release();


	return 0;
}
