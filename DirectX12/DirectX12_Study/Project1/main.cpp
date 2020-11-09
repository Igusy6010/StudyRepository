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

// ウィンドウサイズ
const int window_width = 1280;
const int window_height = 768;

// 基本オブジェクト
ID3D12Device* _dev = nullptr;
IDXGIFactory6* _dxgiFactory = nullptr;
IDXGISwapChain4* _swapchain = nullptr;

// コマンドリストを作成する
ID3D12CommandAllocator* _cmdAllocator = nullptr;
ID3D12GraphicsCommandList* _cmdList = nullptr;
ID3D12CommandQueue* _cmdQueue = nullptr;

// ディスクリプタヒープ
ID3D12DescriptorHeap* _rtvHeaps = nullptr;

// バックバッファー
vector<ID3D12Resource*> _backBuffers;


/// <summary>
/// DirectX12 を初期化 
/// </summary>
void InitializeDirectX12(HWND hwnd)
{
#ifdef _DEBUG
	auto result = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&_dxgiFactory));
#else
	auto result = CreateDXGIFactory1(IID_PPV_ARGS(&_dxgiFactory));
#endif

	// アダプターの列挙用
	vector<IDXGIAdapter*> adapters;

	// ここに特定の名前を持つアダプターオブジェクトが入る
	IDXGIAdapter* tmpAdapter = nullptr;
	for (int i = 0; _dxgiFactory->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		adapters.push_back(tmpAdapter);
	}

	for (auto adpt : adapters)
	{
		DXGI_ADAPTER_DESC adesc = {};
		adpt->GetDesc(&adesc);	// アダプターの説明オブジェクト取得

		wstring strDesc = adesc.Description;

		// 探したいアダプターの名前を確認
		if (strDesc.find(L"NVIDIA") != std::string::npos)
		{
			tmpAdapter = adpt;
			break;
		}
	}


	// フューチャーレベルを最新から見ていく
	D3D_FEATURE_LEVEL levels[] = {
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};

	// Direct3D デバイスの初期化
	D3D_FEATURE_LEVEL featureLevel;
	for (auto lv : levels)
	{
		if (D3D12CreateDevice(tmpAdapter, lv, IID_PPV_ARGS(&_dev)) == S_OK)
		{
			featureLevel = lv;
			break;	// 生成可能なバージョンが見つかったループを打ち切る
		}
	}

	// アダプターの初期化
	for (auto adapter : adapters)
	{
		adapter->Release();
	}
	vector<IDXGIAdapter*>().swap(adapters);

	if (!_dev)
	{
		// CreateDevice に失敗したら、アプリケーションを終了させる
		PostQuitMessage(0);
	}

	// コマンドリストを作成する
	result = _dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_cmdAllocator));
	if (result != S_OK)
	{
		// CreateCommandAllocator に失敗したら、アプリケーションを終了させる
		PostQuitMessage(0);
	}

	result = _dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _cmdAllocator, nullptr, IID_PPV_ARGS(&_cmdList));
	if (result != S_OK)
	{
		// CreateCommandList に失敗したら、アプリケーションを終了させる
		PostQuitMessage(0);
	}


	// コマンドキューの準備
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};

	// タイムアウトなし
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

	// アダプターを1つしか使わないときは 0 でよい
	cmdQueueDesc.NodeMask = 0;

	// プライオリティは特に指定なし
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;

	// コマンドリストと合わせる
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	// キュー生成
	result = _dev->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&_cmdQueue));
	if (result != S_OK)
	{
		// CreateCommandQueue に失敗したら、アプリケーションを終了させる
		PostQuitMessage(0);
	}


	//スワップチェーン設定
	DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
	swapchainDesc.Width = window_width;
	swapchainDesc.Height = window_width;
	swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapchainDesc.Stereo = false;
	swapchainDesc.SampleDesc.Count = 1;
	swapchainDesc.SampleDesc.Quality = 0;
	swapchainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
	swapchainDesc.BufferCount = 2;

	// バックバッファーは伸び縮み可能
	swapchainDesc.Scaling = DXGI_SCALING_STRETCH;

	// フリップ後は速やかに破棄
	swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	// 特になし
	swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;

	// ウィンドウ<=>フルスクリーン切り替え可能
	swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	result = _dxgiFactory->CreateSwapChainForHwnd(_cmdQueue, hwnd, &swapchainDesc, nullptr, nullptr, (IDXGISwapChain1**)&_swapchain);
	if (result != S_OK)
	{
		// CreateSwapChainForHwnd に失敗したら、アプリケーションを終了させる
		PostQuitMessage(0);
	}


	// ディスクリプタヒープを作成
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;	// レンダーターゲットビューなのでRTV
	heapDesc.NodeMask = 0;
	heapDesc.NumDescriptors = 2;	// 表・裏の2つ
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;	// 特になし

	result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&_rtvHeaps));
	if (result != S_OK)
	{
		// CreateDescriptorHeap に失敗したら、アプリケーションを終了させる
		PostQuitMessage(0);
	}

	// スワップチェーン
	DXGI_SWAP_CHAIN_DESC swcDesc = {};
	result = _swapchain->GetDesc(&swcDesc);
	if (result != S_OK)
	{
		// CreateDescriptorHeap に失敗したら、アプリケーションを終了させる
		PostQuitMessage(0);
	}

	// バックバッファー作成
	_backBuffers.resize(swcDesc.BufferCount);
	D3D12_CPU_DESCRIPTOR_HANDLE handle = _rtvHeaps->GetCPUDescriptorHandleForHeapStart();	// 先頭のアドレスを取得する
	for (unsigned int idx = 0; idx < swcDesc.BufferCount; ++idx)
	{
		// スワップチェーン内のバッファーとビューの関連付け
		result = _swapchain->GetBuffer(idx, IID_PPV_ARGS(&_backBuffers[idx]));
		if (result != S_OK)
		{
			// GetBuffer に失敗したら、アプリケーションを終了させる
			PostQuitMessage(0);
		}

		// レンダーターゲットビューを生成する
		_dev->CreateRenderTargetView(_backBuffers[idx], nullptr, handle);

		// ポインターをずらす
		handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}
}

/// <summary>
/// コンソール画面にフォーマット付き文字列を表示
/// </summary>
/// <param name="format"></param>
/// <param name="">可変長引数</param>
/// <remarks>この関数はデバッグ用です。デバッグ時にしか動作しません</remarks>
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
/// 面倒だけど書かなければいけない関数 
/// </summary>
/// <param name="hwnd"></param>
/// <param name="msg"></param>
/// <param name="wparam"></param>
/// <param name="lparam"></param>
/// <returns></returns>
LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	// ウィンドウが破棄されたら呼ばれる
	if (msg == WM_DESTROY)
	{
		PostQuitMessage(0); // OSに対して「もうこのアプリは終わる」と伝える
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

void EnableDebugLayer()
{
	ID3D12Debug* debugLayer = nullptr;
	auto result = D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer));

	debugLayer->EnableDebugLayer();	// デバッグレイヤーを有効にする
	debugLayer->Release();	// 有効化したらインターフェイスを解放する
}

/// <summary>
/// エントリポイント
/// </summary>
#ifdef _DEBUG
int main()
#else
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
#endif // _DEBUG
{
//	DebugOutputFormatString("Show window test.");
//	getchar();

	// ウィンドウクラスの生成&登録
	WNDCLASSEX w = {};
	w.cbSize = sizeof(WNDCLASSEX);
	w.lpfnWndProc = (WNDPROC)WindowProcedure;	// コールバック関数の指定
	w.lpszClassName = _T("DX12Sample");			// アプリケーションクラス名(適当で良い)
	w.hInstance = GetModuleHandle(nullptr);		// ハンドルの取得

	// アプリケーションクラス(ウィンドウクラスの指定をOSに伝える)
	RegisterClassEx(&w);

	// ウィンドウサイズを決める
	RECT wrc = {
		0,
		0,
		window_width,
		window_height
	};

	// 関数を使ってウィンドウのサイズを補正する
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	// ウィンドウオブジェクトの生成
	HWND hwnd = CreateWindow(
		w.lpszClassName,		// クラス名の指定
		_T("DX12テスト"),		// タイトルバーの文字
		WS_OVERLAPPEDWINDOW,	// タイトルバーと境界線があるウィンドウ
		CW_USEDEFAULT,			// 表示x座標はOSにおまかせ
		CW_USEDEFAULT,			//  〃 y座標はOSにおまかせ
		wrc.right - wrc.left,	// ウィンドウ幅
		wrc.bottom - wrc.top,	// ウィンドウ高さ
		nullptr,				// 親ウィンドウハンドル
		nullptr,				// メニューハンドル
		w.hInstance,			// 呼び出しアプリケーションハンドル
		nullptr					// 追加パラメーター
		);

#ifdef _DEBUG
	// デバッグレイヤーをオンに
	EnableDebugLayer();
#endif

	// DirectX12の初期化
	InitializeDirectX12(hwnd);

	// フェンス初期化
	ID3D12Fence* _fence = nullptr;
	UINT64 _fenceVal = 0;
	auto result = _dev->CreateFence(_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));

	// ウィンドウの表示
	ShowWindow(hwnd, SW_SHOW);

	// 1フレームで行うべき処理たち
	{
		auto bbIdx = _swapchain->GetCurrentBackBufferIndex();

		// リソースバリアを設定する
		D3D12_RESOURCE_BARRIER BarrierDesc = {};
		BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;	// 遷移
		BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;	// 特に指定なし
		BarrierDesc.Transition.pResource = _backBuffers[bbIdx];	// バックバッファーリソース
		BarrierDesc.Transition.Subresource = 0;

		BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;	// 直前はPRESENT状態
		BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;	// 今からレンダーターゲット状態
		_cmdList->ResourceBarrier(1, &BarrierDesc);	// バリア指定実行

		// レンダーターゲットを設定
		auto rtvH = _rtvHeaps->GetCPUDescriptorHandleForHeapStart();
		rtvH.ptr += bbIdx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		_cmdList->OMSetRenderTargets(1, &rtvH, true, nullptr);

		// 画面クリア
		float clearColor[] = { 1.0f, 1.0f, 0.0f, 1.0f }; // 黄色
		_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);

		BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		_cmdList->ResourceBarrier(1, &BarrierDesc);	// バリア指定実行

		// 命令のクローズ
		_cmdList->Close();

		// コマンドリストの実行
		ID3D12CommandList* cmdLists[] = { _cmdList };
		_cmdQueue->ExecuteCommandLists(1, cmdLists);

		// コマンドの完了待ち
		_cmdQueue->Signal(_fence, ++_fenceVal);

		//while (_fence->GetCompletedValue() != _fenceVal)
		//{
		//	;	// ビジーループ
		//}
		// ビジーループを使わない方法
		if (_fence->GetCompletedValue() != _fenceVal)
		{
			// イベントハンドルの取得
			auto event = CreateEvent(nullptr, false, false, nullptr);

			_fence->SetEventOnCompletion(_fenceVal, event);

			// イベントが発生するまで待ち続ける(INFINITE)
			WaitForSingleObject(event, INFINITE);

			// イベントハンドルを閉じる
			CloseHandle(event);
		}

		_cmdAllocator->Reset();	// キューをクリア
		_cmdList->Reset(_cmdAllocator, nullptr);	// 再びコマンドリストを溜める準備

		// フリップ
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

		// アプリケーションが終わるときにmessage が WM_QUIT になる
		if (msg.message == WM_QUIT)
		{
			break;
		}
	}

	// もうクラスは使わないので登録を解除する
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
