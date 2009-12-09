#include "stdafx.h"
#include <d3d10.h>
#include <d3dx10.h>

#include "window-video.h"

static HWND         g_hVideo = NULL;

D3D10_DRIVER_TYPE       g_driverType = D3D10_DRIVER_TYPE_NULL;
ID3D10Device*           g_pd3dDevice = NULL;
IDXGISwapChain*         g_pSwapChain = NULL;
ID3D10RenderTargetView* g_pRenderTargetView = NULL;

#define MAX_LOADSTRING 100

// Video_WndProc
LRESULT CALLBACK Video_WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{ int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
		case WM_PAINT:
			hdc = BeginPaint(hWnd, &ps);
			EndPaint(hWnd, &ps);
			break;

    case WM_CLOSE:
      { ShowWindow( hWnd, FALSE );
      }
	    break;
	    
    case WM_DESTROY:
      { PostQuitMessage(0);
      }
		  break;

		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

ATOM Video_RegisterClass ( HINSTANCE hInstance )
{   //
    // Register class
    //
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = Video_WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, (LPCTSTR)IDI_FETCH);
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = NULL;
    wcex.lpszClassName  = VIDEO_CLASS_NAME;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_FETCH);
    if( !RegisterClassEx(&wcex) )
      return E_FAIL;
    return S_OK;
}

HWND Video_InitWindow ( HINSTANCE hInstance, int nCmdShow)
{ HWND hWnd;   
  TCHAR        szTitle[MAX_LOADSTRING] = "Video:\0";  // The title bar text
	 
  LoadString(hInstance, IDS_APP_TITLE, szTitle+6, MAX_LOADSTRING-6);
  
  if( FAILED( Video_RegisterClass(hInistance) ) )
    return E_FAIL;
    
  //
  // Create window
  //  
  RECT rc = { 0, 0, 640, 480 };
  AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, FALSE );
  g_hVideo = CreateWindow( VIDEO_CLASS_NAME, 
                           "Direct3D 10 Tutorial 0: Setting Up Window",
                           WS_OVERLAPPEDWINDOW,
                           CW_USEDEFAULT,
                           CW_USEDEFAULT, 
                           rc.right - rc.left,
                           rc.bottom - rc.top, 
                           NULL,
                           NULL,
                           hInstance,
                           NULL);

  if( !g_hVideo )
      return FALSE;

  ShowWindow( g_hVideo, nCmdShow );
}

//--------------------------------------------------------------------------------------
// Create Direct3D device and swap chain
//--------------------------------------------------------------------------------------
HRESULT InitDevice()
{
    HRESULT hr = S_OK;;

    RECT rc;
    GetClientRect( g_hVideo, &rc );
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D10_CREATE_DEVICE_DEBUG;
#endif

    D3D10_DRIVER_TYPE driverTypes[] =
    {
        D3D10_DRIVER_TYPE_HARDWARE,
        D3D10_DRIVER_TYPE_REFERENCE,
    };
    UINT numDriverTypes = sizeof( driverTypes ) / sizeof( driverTypes[0] );

    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory( &sd, sizeof( sd ) );
    sd.BufferCount = 1;
    sd.BufferDesc.Width = width;
    sd.BufferDesc.Height = height;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = g_hVideo;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;

    for( UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++ )
    {
        g_driverType = driverTypes[driverTypeIndex];
        hr = D3D10CreateDeviceAndSwapChain( NULL,             //adapter
                                            g_driverType,     //driver type
                                            NULL,             //software module
                                            createDeviceFlags,//flags   -  optionally set debug mode
                                            D3D10_SDK_VERSION,//version
                                            &sd,              //swap chain desciption
                                            &g_pSwapChain,    //swap chain
                                            &g_pd3dDevice );  //device
        if( SUCCEEDED( hr ) )
            break;
    }
    if( FAILED( hr ) )
        return hr;

    // Create a render target view
    ID3D10Texture2D* pBackBuffer;
    hr = g_pSwapChain->GetBuffer( 0, __uuidof( ID3D10Texture2D ), ( LPVOID* )&pBackBuffer );
    if( FAILED( hr ) )
        return hr;

    hr = g_pd3dDevice->CreateRenderTargetView( pBackBuffer, NULL, &g_pRenderTargetView );    
    pBackBuffer->Release();
    if( FAILED( hr ) )
        return hr;

    g_pd3dDevice->OMSetRenderTargets( 1, &g_pRenderTargetView, NULL );

    // Setup the viewport
    D3D10_VIEWPORT vp;
    vp.Width = width;
    vp.Height = height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    g_pd3dDevice->RSSetViewports( 1, &vp );

    return S_OK;
}

DWORD WINAPI Video_Main_Thread( LPVOID lparam )
{	MSG msg = {0};

  if( FAILED( InitWindow( hInstance, nCmdShow ) ) )
      return 0;

  if( FAILED( InitDevice() ) )
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
      else
      {
          Render();
      }
  }

  CleanupDevice();

  return ( int )msg.wParam;
}