//--------------------------------------------------------------------------------------
// File: ProjectX.cpp
//
// This application displays a triangle using Direct3D 11
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include <windows.h>
#include <d3d11.h>
#include <d3dx11.h>
#include <d3dcompiler.h>	//Динамическое создание шейдеров
#include <xnamath.h>
#include <string>
#include <sstream>
#include <tchar.h>
#include "resource.h"


//--------------------------------------------------------------------------------------
// Structures
//--------------------------------------------------------------------------------------
struct SimpleVertex
{
    XMFLOAT3 Pos;	//XMFLOAT - структура из 3-x значений типа float;
	XMFLOAT4 Color; //Информацию о цвете
};

struct ConstantBuffer
{
	XMMATRIX mWorld;		// Матрица мира
	XMMATRIX mView;			// Матрица вида
	XMMATRIX mProjection;	// Матрица проекции
};


//--------------------------------------------------------------------------------------
// Глобальные переменные
//--------------------------------------------------------------------------------------
HINSTANCE               g_hInst = NULL;		//идентификаторы приложения
HWND                    g_hWnd = NULL;		//и окна
D3D_DRIVER_TYPE         g_driverType = D3D_DRIVER_TYPE_NULL;		//вычисления в видеокарте или в цп
D3D_FEATURE_LEVEL       g_featureLevel = D3D_FEATURE_LEVEL_11_0;	//версия DirectX поддерживаемая видеокартой
ID3D11Device*           g_pd3dDevice = NULL;		//созданием ресурсов
ID3D11DeviceContext*    g_pImmediateContext = NULL;	//вывод графической информации
IDXGISwapChain*         g_pSwapChain = NULL;		//работа с буферами рисования и выводом нарисованного на экран
ID3D11RenderTargetView* g_pRenderTargetView = NULL;	//объект заднего буфера
ID3D11Texture2D*        g_pDepthStencil = NULL;		//Текстура буфера глубин
ID3D11DepthStencilView* g_pDepthStencilView = NULL;	//Объект вида, буфер глубин

ID3D11VertexShader*     g_pVertexShader = NULL;		//Вершинный шейдер
ID3D11PixelShader*      g_pPixelShader = NULL;		//Пиксельный шейдер
ID3D11InputLayout*      g_pVertexLayout = NULL;		//Описание формата вершин
ID3D11Buffer*           g_pVertexBuffer = NULL;		//Буфер вершин
ID3D11Buffer*			g_pIndexBuffer = NULL;		//Буффер Индексов вершин
ID3D11Buffer*           g_pVertexBuffer2 = NULL;	//Буфер вершин
ID3D11Buffer*			g_pIndexBuffer2 = NULL;		//Буффер Индексов вершин
ID3D11Buffer*			g_pConstantBuffer = NULL;	//Константный буффер

XMMATRIX				g_World;					//Матрица мира
XMMATRIX				g_View;						//Матрица вида
XMMATRIX				g_Projection;				//Матрица проекции

struct{
	UINT numberOfObjects = 5;
	FLOAT orbitRadius = 3;
	BOOL move = false;
	BOOL stop = false;
	FLOAT cameraShift = 0;
} transform;

//--------------------------------------------------------------------------------------
// Декларация функций
//--------------------------------------------------------------------------------------
HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow );		// Создание окна
HRESULT InitDevice();											// Инициализация устройств DirectX
HRESULT InitGeometry();											// Инициализация шаблона ввода и буфера вершин
HRESULT InitMatrixes();											// Инициализация матриц
void SetMatrixes(float fAngle);									// Обновление матрицы мира
void SetMatrixes2();
void Render();													// Функция рисования
void CleanupDevice();											// Удаление созданнных устройств DirectX	
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);		// Функция окна
HRESULT MoveCamera(FLOAT cameraShiftX, FLOAT cameraShiftY, FLOAT cameraShiftZ);


//--------------------------------------------------------------------------------------
// Точка входа в программу. Здесь мы все инициализируем и входим в цикл сообщений.
// Время простоя используем для вызова функции рисования.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
    UNREFERENCED_PARAMETER( hPrevInstance );
    UNREFERENCED_PARAMETER( lpCmdLine );

	// Создание окна приложения
    if( FAILED( InitWindow( hInstance, nCmdShow ) ) )
        return 0;
	
	// Создание объектов DirectX
    if( FAILED( InitDevice() ) )
    {
        CleanupDevice();
        return 0;
    }

	// Создание шейдеров и буфера вершин
	if ( FAILED( InitGeometry() ) )
	{
		CleanupDevice();
		return 0;
	}

	// Инициализация матриц
	if (FAILED( InitMatrixes() ) )
	{
		CleanupDevice();
		return 0;
	}

    // Main message loop
    MSG msg = {0};
    while( WM_QUIT != msg.message )
    {
        if( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
		else				//Если сообщений нет
        {
            Render();		//Рисуем
        }
    }

    CleanupDevice();		//Освобождаем объекты DirectX

    return ( int )msg.wParam;
}


//--------------------------------------------------------------------------------------
// Регистрация класса и создание окна
//--------------------------------------------------------------------------------------
HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow )
{
    // Register class
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof( WNDCLASSEX );
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, (LPCTSTR)IDI_TUTORIAL1);
    wcex.hCursor = LoadCursor( NULL, IDC_ARROW );
    wcex.hbrBackground = ( HBRUSH )( COLOR_WINDOW + 1 );
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = "ProjectXWindowClass";
	wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_TUTORIAL1);
    if( !RegisterClassEx( &wcex ) )
        return E_FAIL;

    // Create window
    g_hInst = hInstance;
    RECT rc = { 0, 0, 640, 480 };
    AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, FALSE );
    g_hWnd = CreateWindow( "ProjectXWindowClass", "ProjectX",
                           WS_OVERLAPPEDWINDOW,
                           CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInstance,
                           NULL );
    if( !g_hWnd )
        return E_FAIL;

    ShowWindow( g_hWnd, nCmdShow );

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Вызывается каждый раз, когда приложение получает сообщение
//--------------------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;
	std::ostringstream oss;

	switch (message)
	{
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_MOUSEMOVE:
		oss << "ProjectX   (" << MAKEPOINTS(lParam).x << ", " << MAKEPOINTS(lParam).y << ")";
		SetWindowText(hWnd, oss.str().c_str());
		break;

	case WM_MOUSEWHEEL:
		if (GET_WHEEL_DELTA_WPARAM(wParam) > 0)
			MoveCamera(0.0f, 0.0f, 1.0f);
		else MoveCamera(0.0f, 0.0f, -1.0f);
		break;

	case WM_KEYDOWN: 
		switch (wParam)
		{
		case VK_ADD:
			if(transform.numberOfObjects <= 10)
				transform.numberOfObjects++;
			break;
		case VK_SUBTRACT:
			if (transform.numberOfObjects > 1)
				transform.numberOfObjects--;
			break;
		case 0x4D: //VK_M
			transform.move = !transform.move;
			break;
		case 0x53: //VK_S
			transform.stop = !transform.stop;
			break;

		case VK_UP:
			MoveCamera(0.0f, 1.0f, 0.0f);
			break;
		case VK_DOWN:
			MoveCamera(0.0f, -1.0f, 0.0f);
			break;
		case VK_RIGHT:
			MoveCamera(1.0f, 0.0f, 0.0f);
			break;
		case VK_LEFT:
			MoveCamera(-1.0f, 0.0f, 0.0f);
			break;

		default:
			break;
		}
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}


//--------------------------------------------------------------------------------------
// Вспомогательная функция для компиляции шейдеров в D3DX11
//--------------------------------------------------------------------------------------
HRESULT CompileShaderFromFile(LPCSTR szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut)
{
	HRESULT hr = S_OK;
	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;

	/*
	#if defined( DEBUG ) || defined( _DEBUG )
		// Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
		// Setting this flag improves the shader debugging experience, but still allows
		// the shaders to be optimized and to run exactly the way they will run in
		// the release configuration of this program.
		dwShaderFlags |= D3DCOMPILE_DEBUG;
	#endif
	*/

	ID3DBlob* pErrorBlob;
	hr = D3DX11CompileFromFile(szFileName, NULL, NULL, szEntryPoint, szShaderModel,
		dwShaderFlags, 0, NULL, ppBlobOut, &pErrorBlob, NULL);
	if (FAILED(hr))
	{
		if (pErrorBlob != NULL)
			OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
		if (pErrorBlob) pErrorBlob->Release();
		return hr;
	}
	if (pErrorBlob) pErrorBlob->Release();

	return S_OK;
}


//--------------------------------------------------------------------------------------
// Создание устройства Direct3D (D3D Device), связующей цепи (Swap Chain) и
// контекста устройства (Immediate Context).
//--------------------------------------------------------------------------------------
HRESULT InitDevice()
{
    HRESULT hr = S_OK;

    RECT rc;
    GetClientRect( g_hWnd, &rc );
    UINT width = rc.right - rc.left;	//получаем ширину
    UINT height = rc.bottom - rc.top;	//и высоту окна

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	//далее понадобится для проверки поддержки хардварной обработки
    D3D_DRIVER_TYPE driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };
    UINT numDriverTypes = ARRAYSIZE( driverTypes );

	// Создаем список поддерживаемых версий DirectX
    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
	UINT numFeatureLevels = ARRAYSIZE( featureLevels );

	
	//Создаем устройства DirectX. Для начала заполним структуру,
	//которая описывает свойства переднего буфера и привязывает его к нашему окну.
    DXGI_SWAP_CHAIN_DESC sd;							// Структура, описывающая цепь связи (Swap Chain)
    ZeroMemory( &sd, sizeof( sd ) );					// очищаем ее
    sd.BufferCount = 1;									// у нас один буфер
    sd.BufferDesc.Width = width;						// ширина буфера
    sd.BufferDesc.Height = height;						// высота буфера
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;	// формат пикселя буфере
    sd.BufferDesc.RefreshRate.Numerator = 60;			// частота обновления экрана
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;	// назначение буфера - задний буфер
    sd.OutputWindow = g_hWnd;							// привязываем к нашему окну
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;									// не полноэкранный режим

    //тот самый цикл для выбора обработки
	for( UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++ )
    {
        g_driverType = driverTypes[driverTypeIndex];
        hr = D3D11CreateDeviceAndSwapChain( NULL, g_driverType, NULL, createDeviceFlags, featureLevels, numFeatureLevels,
                                            D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext );
        if( SUCCEEDED( hr ) )	// Если устройства созданы успешно, то выходим из цикла
            break;
    }
    if( FAILED( hr ) )
        return hr;

	//Создаем задний буфер. !!! В SDK
	//RenderTargetOutput - это передний буфер, а RenderTargetView - задний.
    ID3D11Texture2D* pBackBuffer = NULL;
    hr = g_pSwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), ( LPVOID* )&pBackBuffer );
    if( FAILED( hr ) )
        return hr;

	//Интерфейс g_pd3dDevice будет использоваться для создания остальных объектов
	//По полученному описанию создаем поверхность рисования
    hr = g_pd3dDevice->CreateRenderTargetView( pBackBuffer, NULL, &g_pRenderTargetView );
    pBackBuffer->Release();
    if( FAILED( hr ) )
        return hr;

	// Переходим к созданию буфера глубин
	// Создаем текстуру-описание буфера глубин
	D3D11_TEXTURE2D_DESC descDepth;						// Структура с параметрами
	ZeroMemory(&descDepth, sizeof(descDepth));
	descDepth.Width = width;							// ширина и
	descDepth.Height = height;							// высота текстуры
	descDepth.MipLevels = 1;							// уровень интерполяции
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;	// формат (размер пикселя)
	descDepth.SampleDesc.Count = 1;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;		// вид - буфер глубин
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;
	// При помощи заполненной структуры-описания создаем объект текстуры
	hr = g_pd3dDevice->CreateTexture2D(&descDepth, NULL, &g_pDepthStencil);
	if ( FAILED( hr ) ) 
		return hr;

	// Теперь надо создать сам объект буфера глубин
	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;				// Структура с параметрами
	ZeroMemory(&descDSV, sizeof(descDSV));
	descDSV.Format = descDepth.Format;					// формат как в текстуре
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;
	// При помощи заполненной структуры-описания и текстуры создаем объект буфера глубин
	hr = g_pd3dDevice->CreateDepthStencilView(g_pDepthStencil, &descDSV, &g_pDepthStencilView);
	if (FAILED(hr)) 
		return hr;
	
	//Подключаем объект заднего буфера к контексту устройства
    g_pImmediateContext->OMSetRenderTargets( 1, &g_pRenderTargetView, g_pDepthStencilView);

	//Настройка вьюпорта (масштаб и система координат).
    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)width;
    vp.Height = (FLOAT)height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
	// Подключаем вьюпорт к контексту устройства
    g_pImmediateContext->RSSetViewports( 1, &vp );

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Создание буфера вершин, шейдеров (shaders) и описания формата вершин (input layout)
//--------------------------------------------------------------------------------------
HRESULT InitGeometry()
{
	HRESULT hr = S_OK;

	// Компиляция вершинного шейдера из файла
	ID3DBlob* pVSBlob = NULL; // Вспомогательный объект - просто место в оперативной памяти
	hr = CompileShaderFromFile("ProjectX.fx", "VS", "vs_4_0", &pVSBlob);
	if (FAILED(hr))
	{
		MessageBox(NULL, "The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", "Error", MB_OK);
		return hr;
	}

	// Создание вершинного шейдера
	hr = g_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &g_pVertexShader);
	if (FAILED(hr))
	{
		pVSBlob->Release();
		return hr;
	}

	// Определение шаблона вершин
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		/* семантическое имя, семантический индекс, размер, входящий слот (0-15), адрес начала данных в буфере вершин,
		класс входящего слота (не важно), InstanceDataStepRate (не важно) */
	};
	UINT numElements = ARRAYSIZE(layout);

	// Создание шаблона вершин
	hr = g_pd3dDevice->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(),
		pVSBlob->GetBufferSize(), &g_pVertexLayout);
	pVSBlob->Release();
	if (FAILED(hr))
		return hr;

	// Подключение шаблона вершин
	g_pImmediateContext->IASetInputLayout(g_pVertexLayout);

	// Компиляция пиксельного шейдера из файла
	ID3DBlob* pPSBlob = NULL;
	hr = CompileShaderFromFile("ProjectX.fx", "PS", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(NULL,
			"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", "Error", MB_OK);
		return hr;
	}

	// Создание пиксельного шейдера
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &g_pPixelShader);
	pPSBlob->Release();
	if (FAILED(hr))
		return hr;


	// Создание буфера вершин (три вершины треугольника)
	SimpleVertex vertices[] =
	{
		{ XMFLOAT3(0.0f,  1.5f,  0.0f), XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(-1.0f,  0.0f, -1.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(1.0f,  0.0f, -1.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(-1.0f,  0.0f,  1.0f), XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f) },
		{ XMFLOAT3(1.0f,  0.0f,  1.0f), XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f) },
		{ XMFLOAT3(0.0f,  -1.5f,  0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) }
	};

	SimpleVertex vertices2[] =
	{
		//Основа
		{ XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f)  },
		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f)  },
		{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f)  },
		{ XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f)  },
		{ XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f)  },
		{ XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f)  },
		{ XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f)  },
		{ XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f)  },

		//Шипы
		{ XMFLOAT3(0.0f, 0.0f, 3.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f)  },
		{ XMFLOAT3(-3.0f, 0.0f, 0.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f)  },
		{ XMFLOAT3(0.0f, 0.0f, -3.0f), XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f)  },
		{ XMFLOAT3(3.0f, 0.0f, 0.0f), XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f)  },
		{ XMFLOAT3(0.0f, -3.0f, 0.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f)  },
		{ XMFLOAT3(0.0f, 3.0f, 0.0f), XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f)  },
	};

	D3D11_BUFFER_DESC bd;						// Структура, описывающая создаваемый буфер
	ZeroMemory(&bd, sizeof(bd));				// очищаем ее
	bd.Usage = D3D11_USAGE_DEFAULT;
	//bd.ByteWidth = sizeof(SimpleVertex) * 5;	// размер буфера = размер одной вершины * количество вершин
	bd.ByteWidth = sizeof(vertices);
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;	// тип буфера - буфер вершин
	bd.CPUAccessFlags = 0;	

	D3D11_SUBRESOURCE_DATA InitData;			// Структура, содержащая данные буфера
	ZeroMemory(&InitData, sizeof(InitData));	// очищаем ее
	InitData.pSysMem = vertices;				// указатель на наши 3 вершины

	// Вызов метода g_pd3dDevice создаст объект буфера вершин ID3D11Buffer
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pVertexBuffer);
	if (FAILED(hr))
		return hr;

	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(vertices2);
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = vertices2;

	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pVertexBuffer2);
	if (FAILED(hr))
		return hr;

	// Создание буфера индексов
	// Создание массива с данными
	WORD indices[] =
	{				
		0,2,1,      // Треугольник 1 = vertices[0], vertices[2], vertices[1] 
		0,3,4,      // Треугольник 2 = vertices[0], vertices[3], vertices[4] 
		0,1,3,      // и т. д. 
		0,4,2,

		5,1,2,
		5,4,3,
		5,3,1,
		5,2,4,
	};// индексы массива vertices[], по которым строятся треугольники

	WORD indices2[] =
	{
		8, 0, 3,
		8, 3, 7,
		8, 7, 4,
		8, 4, 0,
		
		9, 1, 0,
		9, 0, 4,
		9, 4, 5,
		9, 5, 1,

		10, 2, 1,
		10, 1, 5,
		10, 5, 6,
		10, 6, 2,

		11, 3, 2,
		11, 2, 6,
		11, 6, 7,
		11, 7, 3,

		12, 3, 0,
		12, 0, 1,
		12, 1, 2,
		12, 2, 3,

		13, 4, 7,
		13, 7, 6,
		13, 6, 5,
		13, 5, 4,
	};// индексы массива vertices[], по которым строятся треугольники

	bd.Usage = D3D11_USAGE_DEFAULT;				// Структура, описывающая создаваемый буфер
	//bd.ByteWidth = sizeof(WORD) * 18;			// для 6 треугольников необходимо 18 вершин
	bd.ByteWidth = sizeof(indices);
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;		// тип - буфер индексов
	bd.CPUAccessFlags = 0;
	InitData.pSysMem = indices;					// указатель на наш массив индексов
	
	// Вызов метода g_pd3dDevice создаст объект буфера индексов
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pIndexBuffer);
	if (FAILED(hr)) 
		return hr;

	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(indices2);
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;
	InitData.pSysMem = indices2;

	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pIndexBuffer2);
	if (FAILED(hr))
		return hr;

	// Установка буфера вершин
	UINT stride = sizeof(SimpleVertex);
	UINT offset = 0;
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
	// Установка буфера индексов
	g_pImmediateContext->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
	// Установка способа отрисовки вершин в буфере (TRIANGLE LIST - точки 1-3 - первый треугольник 4-6 - второй и т. д.)
	g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Создание константного буфера
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(ConstantBuffer);			// размер буфера = размеру структуры
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;		// тип - константный буфер
	bd.CPUAccessFlags = 0;
	hr = g_pd3dDevice->CreateBuffer(&bd, NULL, &g_pConstantBuffer);
	if (FAILED(hr)) 
		return hr;

	return S_OK;
}


//--------------------------------------------------------------------------------------
// Инициализация матриц
//--------------------------------------------------------------------------------------
HRESULT InitMatrixes()
{
	RECT rc;
	GetClientRect(g_hWnd, &rc);
	UINT width = rc.right - rc.left;			//получаем ширину
	UINT height = rc.bottom - rc.top;			//и высоту окна

	// Инициализация матрицы мира
	g_World = XMMatrixIdentity();

	// Инициализация матрицы вида
	XMVECTOR Eye = XMVectorSet(0.0f, 2.0f, -9.0f, 0.0f);  // Откуда смотрим
	XMVECTOR At = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);    // Куда смотрим
	XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);    // Направление верха
	g_View = XMMatrixLookAtLH(Eye, At, Up);

	// Инициализация матрицы проекции
	g_Projection = XMMatrixPerspectiveFovLH(XM_PIDIV4 , width / (FLOAT)height, 0.01f, 100.0f);

	return S_OK;
}

HRESULT MoveCamera(FLOAT cameraShiftX, FLOAT cameraShiftY, FLOAT cameraShiftZ)
{
	static FLOAT currentShiftX = 0.0f;
	static FLOAT currentShiftY = 2.0f;
	static FLOAT currentShiftZ = -9.0f;
	
	if ((currentShiftX < 10.0f && cameraShiftX > 0)
		|| (currentShiftX > -10.0f && cameraShiftX < 0))
		currentShiftX += cameraShiftX;

	if ((currentShiftY < 10.0f && cameraShiftY > 0)
		|| (currentShiftY > -10.0f && cameraShiftY < 0))
		currentShiftY += cameraShiftY;

	if ((currentShiftZ < -5.0f && cameraShiftZ > 0)
		|| (currentShiftZ > -20.0f && cameraShiftZ < 0))
		currentShiftZ += cameraShiftZ;

	XMVECTOR Eye = XMVectorSet(currentShiftX, currentShiftY, currentShiftZ, 0.0f);
	XMVECTOR At = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	g_View = XMMatrixLookAtLH(Eye, At, Up);

	return S_OK;
}


//--------------------------------------------------------------------------------------
// Обновление матриц
//--------------------------------------------------------------------------------------
void SetMatrixes(float fAngle)
{
	// Обновление переменной-времени
	static float t = 0.0f;
	if (g_driverType == D3D_DRIVER_TYPE_REFERENCE)
	{
		t += (float)XM_PI * 0.0125f;
	}
	else
	{
		static DWORD dwTimeStart = 0;
		DWORD dwTimeCur = GetTickCount();
		if (dwTimeStart == 0)
			dwTimeStart = dwTimeCur;
		if (!transform.stop)
			t = (dwTimeCur - dwTimeStart) / 1000.0f;
	}

	static float orbitBias = 0;
	static int increase = 1;
	if (orbitBias == 0) orbitBias = transform.orbitRadius;
	if (orbitBias > 5) increase = -1;
	if (orbitBias < 1) increase = 1;
	orbitBias += 0.0001 * increase * (transform.move ? 1 : 0);
	// Матрица-орбита: позиция объекта
	XMMATRIX mOrbit = XMMatrixRotationY(-t + fAngle);
	// Матрица-спин: вращение объекта вокруг своей оси
	XMMATRIX mSpin = XMMatrixRotationY(t * 2);
	// Матрица-позиция: перемещение на три единицы влево от начала координат
	XMMATRIX mTranslate = XMMatrixTranslation(orbitBias , 0.0f, 0.0f);
	// Матрица-масштаб: сжатие объекта в 2 раза
	XMMATRIX mScale = XMMatrixScaling(0.3f, 0.5f, 0.3f);

	// Результирующая матрица
	//  --Сначала мы в центре, в масштабе 1:1:1, повернуты по всем осям на 0.0f.
	//  --Сжимаем -> поворачиваем вокруг Y (пока мы еще в центре) -> переносим влево ->
	//  --снова поворачиваем вокруг Y.
	g_World = mScale * mSpin * mTranslate * mOrbit;

	// Обновить константный буфер
	// создаем временную структуру и загружаем в нее матрицы
	ConstantBuffer cb;
	cb.mWorld = XMMatrixTranspose(g_World);
	cb.mView = XMMatrixTranspose(g_View);
	cb.mProjection = XMMatrixTranspose(g_Projection);
	
	// загружаем временную структуру в константный буфер g_pConstantBuffer
	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer, 0, NULL, &cb, 0, 0);
}

void SetMatrixes2()
{
	// Обновление переменной-времени
	static float t = 0.0f;
	if (g_driverType == D3D_DRIVER_TYPE_REFERENCE)
	{
		t += (float)XM_PI * 0.0125f;
	}
	else
	{
		static DWORD dwTimeStart = 0;
		DWORD dwTimeCur = GetTickCount();
		if (dwTimeStart == 0)
			dwTimeStart = dwTimeCur;
		if (!transform.stop)
			t = (dwTimeCur - dwTimeStart) / 1000.0f;
	}

	static float shiftY = 0;
	static int increase = 1;
	if (shiftY > 2) increase = -1;
	if (shiftY < -2) increase = 1;
	shiftY += 0.0005 * increase * (transform.move ? 1 : 0);
	
	// Матрица-спин: вращение объекта вокруг своей оси
	XMMATRIX mSpinX = XMMatrixRotationX(t);
	XMMATRIX mSpinY = XMMatrixRotationY(t);	
	XMMATRIX mSpinZ = XMMatrixRotationZ(t);
	// Матрица-позиция: перемещение вверх-вниз
	XMMATRIX mTranslate = XMMatrixTranslation(0.0f, shiftY, 0.0f);
	// Матрица-масштаб: сжатие объекта в 2 раза
	XMMATRIX mScale = XMMatrixScaling(0.5f, 0.5f, 0.5f);

	// Результирующая матрица
	//  --Сначала мы в центре, в масштабе 1:1:1, повернуты по всем осям на 0.0f.
	//  --Сжимаем -> поворачиваем вокруг Y (пока мы еще в центре) -> переносим влево ->
	//  --снова поворачиваем вокруг Y.
	g_World = mScale *  mSpinX *  mSpinY *  mSpinZ * mTranslate;

	// Обновить константный буфер
	// создаем временную структуру и загружаем в нее матрицы
	ConstantBuffer cb;
	cb.mWorld = XMMatrixTranspose(g_World);
	cb.mView = XMMatrixTranspose(g_View);
	cb.mProjection = XMMatrixTranspose(g_Projection);

	// загружаем временную структуру в константный буфер g_pConstantBuffer
	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer, 0, NULL, &cb, 0, 0);
}


//--------------------------------------------------------------------------------------
// Удалить все созданные объекты
//--------------------------------------------------------------------------------------
void CleanupDevice()
{
	// Сначала отключим контекст устройства
    if( g_pImmediateContext ) g_pImmediateContext->ClearState();
	// Потом удалим объекты
	if (g_pConstantBuffer) g_pConstantBuffer->Release();
    if( g_pVertexBuffer ) g_pVertexBuffer->Release();
	if (g_pIndexBuffer) g_pIndexBuffer->Release();
    if( g_pVertexLayout ) g_pVertexLayout->Release();	
    if( g_pVertexShader ) g_pVertexShader->Release();
    if( g_pPixelShader ) g_pPixelShader->Release();
	if (g_pDepthStencil) g_pDepthStencil->Release();
	if (g_pDepthStencilView) g_pDepthStencilView->Release();
    if( g_pRenderTargetView ) g_pRenderTargetView->Release();
    if( g_pSwapChain ) g_pSwapChain->Release();
    if( g_pImmediateContext ) g_pImmediateContext->Release();
    if( g_pd3dDevice ) g_pd3dDevice->Release();
}


//--------------------------------------------------------------------------------------
// Рисование кадра
//--------------------------------------------------------------------------------------
void Render()
{
	// Очищаем задний буфер
    float ClearColor[4] = { 0.0f, 0.125f, 0.3f, 1.0f }; // red,green,blue,alpha
    g_pImmediateContext->ClearRenderTargetView( g_pRenderTargetView, ClearColor );

	// Очистить буфер глубин до 1.0 (максимальное значение)
	g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);


	UINT stride = sizeof(SimpleVertex);
	UINT offset = 0;
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
	g_pImmediateContext->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
	g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


	for (int i = 0; i < transform.numberOfObjects; i++)
	{
		SetMatrixes(i * XM_PI * 2 / transform.numberOfObjects);
		
		// Подключить к устройству рисования шейдеры
		g_pImmediateContext->VSSetShader(g_pVertexShader, NULL, 0);
		g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer);
		g_pImmediateContext->PSSetShader(g_pPixelShader, NULL, 0);
		// Нарисовать n вершин
		g_pImmediateContext->DrawIndexed(24, 0, 0);
	}

	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer2, &stride, &offset);
	g_pImmediateContext->IASetIndexBuffer(g_pIndexBuffer2, DXGI_FORMAT_R16_UINT, 0);
	g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	SetMatrixes2();

	//g_pImmediateContext->VSSetShader(g_pVertexShader, NULL, 0);
	//g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer);
	//g_pImmediateContext->PSSetShader(g_pPixelShader, NULL, 0);

	g_pImmediateContext->DrawIndexed(72, 0, 0);

	// Вывести в передний буфер (на экран) информацию, нарисованную в заднем буфере.
    g_pSwapChain->Present( 0, 0 );
}
