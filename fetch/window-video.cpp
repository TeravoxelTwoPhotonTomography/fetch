#include "stdafx.h"
#include <d3d10.h>
#include <d3dx10.h>
#include "window-video.h"
#include <stdlib.h> // for rand - for testing - remove when done
#include "asynq.h"
#include "frame.h"
#include "render-colormap.h"

#define VIDEO_WINDOW_TEXTURE_RESOURCE_NAME "tx"
#define VIDEO_WINDOW_PATH_TO_SHADER        "shader.fx"
#define VIDEO_WINDOW_SHADER_TECHNIQUE_NAME "Render"

#if 1
#define VIDEO_DISPLAY_DEBUG_SHADER
#else
#endif

struct t_video_display
{ 
  HWND                                hwnd;
  D3D10_DRIVER_TYPE                   driver_type;
  ID3D10Device                       *device;
  IDXGISwapChain                     *swap_chain;
  ID3D10RenderTargetView             *render_target_view;
  ID3D10Effect                       *effect;
  ID3D10EffectTechnique              *technique;
  ID3D10InputLayout                  *vertex_layout;
  ID3D10Buffer                       *vertices;
  ID3D10Buffer                       *indices;
  ID3D10ShaderResourceView           *texture_resource_view[3];
  ID3D10Texture2D                    *active_texture[3];
  ID3D10EffectShaderResourceVariable *active_texture_shader_resource[3];
  
  Colormap_Resource                  *cmaps[3];
  
  asynq                              *frame_source;
  
  float                               aspect;
};

#define VIDEO_DISPLAY_EMPTY { NULL,\
                              D3D10_DRIVER_TYPE_NULL,\
                              NULL,\
                              NULL,\
                              NULL,\
                              NULL,\
                              NULL,\
                              NULL,\
                              NULL,\
                              NULL,\
                              {NULL, NULL, NULL},\
                              {NULL, NULL, NULL},\
                              {NULL, NULL, NULL},\
                              EMPTY_COLORMAP_RESOURCE,\
                              NULL,\
                              1.0f}

struct t_video_display g_video = VIDEO_DISPLAY_EMPTY;

//--------------------------------------------------------------------------------------
// Register class and create window
//--------------------------------------------------------------------------------------
HRESULT _InitWindow( HINSTANCE hInstance, int nCmdShow )
{
    // Register class
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof( WNDCLASSEX );
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = Video_Display_WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon( hInstance, ( LPCTSTR )IDI_FETCH );
    wcex.hCursor = LoadCursor( NULL, IDC_ARROW );
    wcex.hbrBackground = ( HBRUSH )( COLOR_WINDOW + 1 );
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = "VideoDisplayClass";
    wcex.hIconSm = LoadIcon( wcex.hInstance, ( LPCTSTR )IDI_FETCH );
    if( !RegisterClassEx( &wcex ) )
        return E_FAIL;

    // Create window    
    RECT rc = { 0, 0, 1024, 1024 };
    AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, FALSE );
    g_video.hwnd = CreateWindow(  "VideoDisplayClass", 
                                  "Fetch: Video Display",
                                  WS_OVERLAPPEDWINDOW,
                                  CW_USEDEFAULT,
                                  CW_USEDEFAULT,
                                  rc.right - rc.left,
                                  rc.bottom - rc.top,
                                  NULL,
                                  NULL,
                                  hInstance,
                                  NULL );
    if( !g_video.hwnd )
        return E_FAIL;

    ShowWindow( g_video.hwnd, nCmdShow );

    return S_OK;
}



//--------------------------------------------------------------------------------------
// Create Direct3D device and swap chain
//--------------------------------------------------------------------------------------
inline void
_create_dx10_buffer_default_usage( ID3D10Buffer **ppbuf, D3D10_BIND_FLAG bindflags, void *src, size_t nbytes )
{ D3D10_BUFFER_DESC bd;
  D3D10_SUBRESOURCE_DATA InitData;
  bd.Usage                 = D3D10_USAGE_DEFAULT;
  bd.ByteWidth             = nbytes;
  bd.BindFlags             = bindflags;
  bd.CPUAccessFlags        = 0;
  bd.MiscFlags             = 0;    
  InitData.pSysMem         = src;   // copy in the local vertex data
  Guarded_Assert(SUCCEEDED(
    g_video.device->CreateBuffer( &bd, &InitData, ppbuf ) ));
}

void 
_create_device_and_swap_chain(UINT width,UINT height)
{ UINT createDeviceFlags = 0;  
  D3D10_DRIVER_TYPE driverTypes[] =
  {
      D3D10_DRIVER_TYPE_HARDWARE,
      D3D10_DRIVER_TYPE_REFERENCE,
  };
  UINT numDriverTypes = sizeof( driverTypes ) / sizeof( driverTypes[0] );  
  HRESULT hr = S_OK;
  UINT msaa_quality = 1, msaa_levels = 4;
  
#ifdef _DEBUG
  createDeviceFlags |= D3D10_CREATE_DEVICE_DEBUG;
#endif

  DXGI_SWAP_CHAIN_DESC sd;
  ZeroMemory( &sd, sizeof( sd ) );
  sd.BufferCount                              = 2;
  sd.BufferDesc.Width                         = width;
  sd.BufferDesc.Height                        = height;
  sd.BufferDesc.Format                        = DXGI_FORMAT_R8G8B8A8_UNORM;
  sd.BufferDesc.Scaling                       = DXGI_MODE_SCALING_UNSPECIFIED;
  sd.BufferDesc.RefreshRate.Numerator         = 60;
  sd.BufferDesc.RefreshRate.Denominator       = 1;
  sd.BufferUsage                              = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  sd.OutputWindow                             = g_video.hwnd;  // Binding to window
  sd.SampleDesc.Count                         = msaa_levels;
  sd.SampleDesc.Quality                       = msaa_quality;
  sd.Windowed                                 = TRUE;

  for( UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++ )
  { g_video.driver_type = driverTypes[driverTypeIndex];
    if( SUCCEEDED(
      hr = D3D10CreateDeviceAndSwapChain( NULL, 
                                          g_video.driver_type,
                                          NULL,
                                          createDeviceFlags,                                        
                                          D3D10_SDK_VERSION,
                                          &sd,
                                          &g_video.swap_chain,
                                          &g_video.device ) ))
      break;    
  }
  Guarded_Assert( SUCCEEDED(hr) );
  
  g_video.device->CheckMultisampleQualityLevels( DXGI_FORMAT_R8G8B8A8_UNORM, msaa_levels, &msaa_quality );
  debug("MSAA: %d quality %d\r\n",msaa_levels, msaa_quality );
  
  // Create a render target view - binds the swap chain (and hWnd) to the Output Merger stage.
  ID3D10Texture2D* pBuffer;
  Guarded_Assert(SUCCEEDED( hr = g_video.swap_chain->GetBuffer( 0, __uuidof( ID3D10Texture2D ), ( LPVOID* )&pBuffer ) ));    
  Guarded_Assert(SUCCEEDED( hr = g_video.device->CreateRenderTargetView( pBuffer, NULL, &g_video.render_target_view )    ));
  pBuffer->Release();

  g_video.device->OMSetRenderTargets( 1, &g_video.render_target_view, NULL );
}

inline void 
_copy_data_to_texture2d( ID3D10Texture2D *dst, void* src, size_t nbytes )
{ D3D10_MAPPED_TEXTURE2D mappedTex;
  Guarded_Assert( !FAILED( 
      dst->Map( D3D10CalcSubresource(0, 0, 1), D3D10_MAP_WRITE_DISCARD, 0, &mappedTex ) ));  
  memcpy(mappedTex.pData, src, nbytes);
  dst->Unmap( D3D10CalcSubresource(0, 0, 1) );        
}

inline void 
_copy_data_to_texture2d_ex( ID3D10Texture2D *dst, void* src, Frame_Descriptor *desc, int ichan )
{ D3D10_MAPPED_TEXTURE2D mappedTex;
  Frame_Interface *f = Frame_Descriptor_Get_Interface( desc );
  
  Guarded_Assert( !FAILED( 
      dst->Map( D3D10CalcSubresource(0, 0, 1),
                D3D10_MAP_WRITE_DISCARD,
                0,                               // wait for the gpu 
                &mappedTex ) ));  
  memcpy(mappedTex.pData, 
         f->get_channel(desc,src,ichan), 
         f->get_nbytes(desc));
  dst->Unmap( D3D10CalcSubresource(0, 0, 1) );        
}

inline void
_setup_viewport(UINT width,UINT height)
{ D3D10_VIEWPORT vp;
  vp.Width = width;
  vp.Height = height;
  vp.MinDepth = 0.0f;
  vp.MaxDepth = 1.0f;
  vp.TopLeftX = 0;
  vp.TopLeftY = 0;
  g_video.device->RSSetViewports( 1, &vp );
}

inline void
_setup_geometry(void)
{ struct TVertex { D3DXVECTOR3 Pos; D3DXVECTOR2 Tex; };  
  TVertex vertices[] =
  {   { D3DXVECTOR3( -1.0f, -1.0f, 1.0f ), D3DXVECTOR2( 0.0f, 0.0f ) }, // bottom left
      { D3DXVECTOR3(  1.0f, -1.0f, 1.0f ), D3DXVECTOR2( 1.0f, 0.0f ) }, // bottom right
      { D3DXVECTOR3(  1.0f,  1.0f, 1.0f ), D3DXVECTOR2( 1.0f, 1.0f ) }, // top    right
      { D3DXVECTOR3( -1.0f,  1.0f, 1.0f ), D3DXVECTOR2( 0.0f, 1.0f ) }, // top    left
  };
  DWORD indices[] = { 3,2,0,1 };
  
  // Create the input layout
  { D3D10_PASS_DESC PassDesc;    
    D3D10_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0 , D3D10_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT   , 0, 12, D3D10_INPUT_PER_VERTEX_DATA, 0 },
    };
    UINT numElements = sizeof( layout ) / sizeof( layout[0] );
    g_video.technique->GetPassByIndex( 0 )->GetDesc( &PassDesc );
    Guarded_Assert(SUCCEEDED(  
      g_video.device->CreateInputLayout( layout,
                                         numElements,
                                         PassDesc.pIAInputSignature,
                                         PassDesc.IAInputSignatureSize,
                                        &g_video.vertex_layout ) ));
    g_video.device->IASetInputLayout( g_video.vertex_layout ); // Bind to input-assembler
  }

  // create vertex and index buffers
  _create_dx10_buffer_default_usage( &g_video.vertices, D3D10_BIND_VERTEX_BUFFER, vertices, sizeof(vertices) );
  { UINT stride = sizeof( TVertex );     // Bind vertex buffer to input assembler
    UINT offset = 0;
    g_video.device->IASetVertexBuffers( 0, 1, &g_video.vertices, &stride, &offset );
  }
  
  _create_dx10_buffer_default_usage( &g_video.indices, D3D10_BIND_INDEX_BUFFER, indices, sizeof(indices) );
  g_video.device->IASetIndexBuffer(   g_video.indices, DXGI_FORMAT_R32_UINT, 0 );

  // Set primitive topology
  g_video.device->IASetPrimitiveTopology( D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP );
}

inline void _load_colormaps(void)
{ int i=3, nrows = 1<<16;
  const char* varnames[] = {"cmap0","cmap1","cmap2"};
  while(i--)
  { g_video.cmaps[i] = Colormap_Resource_Alloc();
    Colormap_Resource_Attach( g_video.cmaps[i], nrows, g_video.effect, "cmap0" );
  }  
  Colormap_Gray   ( g_video.cmaps[0], 0.0f, (f32) nrows, nrows );
  Colormap_HSV_Hue( g_video.cmaps[1], 1.0, 1.0, 1.0, 0.0f, (f32) nrows, nrows );
  Colormap_Red    ( g_video.cmaps[2], 0.0f, (f32) nrows, nrows );
}

inline void
_load_shader(const char* path, const char* technique)
{ HRESULT hr = S_OK;
  DWORD dwShaderFlags = D3D10_SHADER_ENABLE_STRICTNESS;
#ifdef VIDEO_DISPLAY_DEBUG_SHADER
    dwShaderFlags |= D3D10_SHADER_DEBUG;
#endif
  hr = D3DX10CreateEffectFromFile( path,
                                   NULL,
                                   NULL,
                                   "fx_4_0",
                                   dwShaderFlags,
                                   0,
                                   g_video.device,
                                   NULL,
                                   NULL,
                                   &g_video.effect,
                                   NULL,
                                   NULL );
  if( FAILED( hr ) )
    error( "Could not compile the FX file.  Perhaps it could not be located.");   

  // Obtain the technique ang get references to variables
  g_video.technique                         = g_video.effect->GetTechniqueByName( technique );
  g_video.active_texture_shader_resource[0] = g_video.effect->GetVariableByName( "tx0" )->AsShaderResource();
  g_video.active_texture_shader_resource[1] = g_video.effect->GetVariableByName( "tx1" )->AsShaderResource();
  g_video.active_texture_shader_resource[2] = g_video.effect->GetVariableByName( "tx2" )->AsShaderResource();  
  Guarded_Assert( g_video.technique );
  Guarded_Assert( g_video.active_texture_shader_resource[0] );
  Guarded_Assert( g_video.active_texture_shader_resource[1] );
  Guarded_Assert( g_video.active_texture_shader_resource[2] );
}

inline void 
_create_texture( ID3D10Texture2D **pptex, DXGI_FORMAT format, UINT width, UINT height )
// Based on: ms-help://MS.VSCC.v90/MS.VSIPCC.v90/MS.Windows_Graphics.August.2009.1033/Windows_Graphics/d3d10_graphics_programming_guide_resources_creating_textures.htm#Creating_Empty_Textures
{ D3D10_TEXTURE2D_DESC desc;
  ZeroMemory( &desc, sizeof(desc) );
  int i = 3;
  desc.Width                         = width;
  desc.Height                        = height;
  desc.MipLevels = desc.ArraySize    = 1;
  desc.Format                        = format;
  desc.SampleDesc.Count              = 1;
  desc.Usage                         = D3D10_USAGE_DYNAMIC;
  desc.CPUAccessFlags                = D3D10_CPU_ACCESS_WRITE;
  desc.BindFlags                     = D3D10_BIND_SHADER_RESOURCE;
  
  while(i--)
    Guarded_Assert( !FAILED( g_video.device->CreateTexture2D( &desc, NULL, g_video.active_texture+i ) ));
}

inline void 
_create_texture_u16( ID3D10Texture2D **pptex, UINT width, UINT height )
{ _create_texture( pptex, DXGI_FORMAT_R16_UNORM, width, height );
}

inline void 
_create_texture_i16( ID3D10Texture2D **pptex, UINT width, UINT height )
{ _create_texture( pptex, DXGI_FORMAT_R16_SNORM, width, height );
}

void
_refresh_active_texture_shader_resource_view(void)
// Create the resource view for textures and bind it to the shader varable
{ D3D10_SHADER_RESOURCE_VIEW_DESC srvDesc;
  D3D10_RESOURCE_DIMENSION type;
  D3D10_TEXTURE2D_DESC desc;
  int i = 3;
  
  g_video.active_texture[0]->GetType( &type );
  Guarded_Assert( type == D3D10_RESOURCE_DIMENSION_TEXTURE2D );
  g_video.active_texture[0]->GetDesc( &desc );

  srvDesc.Format                    = desc.Format;
  srvDesc.ViewDimension             = D3D10_SRV_DIMENSION_TEXTURE2D;
  srvDesc.Texture2D.MipLevels       = desc.MipLevels;
  srvDesc.Texture2D.MostDetailedMip = desc.MipLevels - 1;

  while(i--)
  { 
    Guarded_Assert(SUCCEEDED( 
      g_video.device->CreateShaderResourceView( g_video.active_texture[i], 
                                               &srvDesc, 
                                               &g_video.texture_resource_view[i] ) ));                                               
    Guarded_Assert(SUCCEEDED(
      g_video.active_texture_shader_resource[i]->SetResource( g_video.texture_resource_view[i] ) ));
  }
}
      
HRESULT _InitDevice()
{ HRESULT hr = S_OK;
  RECT rc;
  GetClientRect( g_video.hwnd, &rc );
  UINT width  = 512; //rc.right - rc.left;
  UINT height = 512; //rc.bottom - rc.top;  
    
  _create_device_and_swap_chain(width, height);
  _setup_viewport( width, height );
  _load_shader(VIDEO_WINDOW_PATH_TO_SHADER, VIDEO_WINDOW_SHADER_TECHNIQUE_NAME);
  _setup_geometry();
  
  { _create_texture_i16( g_video.active_texture, width, height );               
    _refresh_active_texture_shader_resource_view();
    
    // Initialize the texture with test data
    { typedef i16 T;
      size_t src_nelem = width*height;      // create the source buffer
      u16 *src = (u16*)calloc(src_nelem,sizeof(T));
      unsigned int i,j;
      for(i=0;i<height;i++)
        for(j=0;j<width;j++)
          src[j + width * i] = (T) j + width * i + rand()/(1<<16);
      _copy_data_to_texture2d( g_video.active_texture[0], src, src_nelem*sizeof(T) );
      _copy_data_to_texture2d( g_video.active_texture[1], src, src_nelem*sizeof(T) );
      _copy_data_to_texture2d( g_video.active_texture[2], src, src_nelem*sizeof(T) );
      free(src);        
    }      
  }                                          
  if( FAILED( hr ) )
      return hr;

  _load_colormaps();
    
    
  return S_OK;
}


//--------------------------------------------------------------------------------------
// Clean up the objects we've created
//--------------------------------------------------------------------------------------
void _CleanupDevice()
{   int i = 3;
    if( g_video.device )                g_video.device->ClearState();

    if( g_video.vertices )              g_video.vertices->Release();
    if( g_video.indices )               g_video.indices->Release();
    if( g_video.vertex_layout )         g_video.vertex_layout->Release();
    
    while(i--)
    {
      if( g_video.texture_resource_view[i] ) g_video.texture_resource_view[i]->Release();
      if( g_video.active_texture[i] )        g_video.active_texture[i]->Release();
      if( g_video.cmaps[i] )
      { Colormap_Resource_Detach( g_video.cmaps[i] );
        Colormap_Resource_Free  ( g_video.cmaps[i] );
      }
      
    }
    
    if( g_video.effect )                g_video.effect->Release();
    if( g_video.render_target_view )    g_video.render_target_view->Release();
    if( g_video.swap_chain )            g_video.swap_chain->Release();
    if( g_video.device )                g_video.device->Release();
}

//--------------------------------------------------------------------------------------
// Set up the device objects (setup after a resize)
//--------------------------------------------------------------------------------------
void _refresh_objects(UINT width, UINT height)
{ int i = 3;
  HRESULT hr;
  // Create a render target view
  { ID3D10Texture2D* pBuffer;
  
    Guarded_Assert(SUCCEEDED( hr = g_video.swap_chain->GetBuffer( 0, __uuidof( ID3D10Texture2D ), ( LPVOID* )&pBuffer ) ));    
    Guarded_Assert(SUCCEEDED( hr = g_video.device->CreateRenderTargetView( pBuffer, NULL, &g_video.render_target_view )    ));
    pBuffer->Release();

    g_video.device->OMSetRenderTargets( 1, &g_video.render_target_view, NULL );
  }
  _setup_viewport( width, height );
  _load_shader(VIDEO_WINDOW_PATH_TO_SHADER, VIDEO_WINDOW_SHADER_TECHNIQUE_NAME);
  _create_texture_i16( g_video.active_texture, width, height );               
  _refresh_active_texture_shader_resource_view();
  
}

//--------------------------------------------------------------------------------------
// Clean up the device objects (prep for a resize)
//--------------------------------------------------------------------------------------
void _invalidate_objects(void)
{   int i = 3;
    int cnt = 0;
    cnt = g_video.render_target_view->Release();    
    cnt = g_video.effect->Release();
    while(i--)
    { Colormap_Resource_Detach  ( g_video.cmaps[i] );
      cnt = g_video.texture_resource_view[i]->Release();
      cnt = g_video.active_texture[i]->Release();      
    }
}



//--------------------------------------------------------------------------------------
// Initializes everything.
//--------------------------------------------------------------------------------------
HWND Video_Display_Attach( HINSTANCE hInstance, int nCmdShow)
{ if( FAILED( _InitWindow( hInstance, nCmdShow ) ) )
    return NULL;

  if( FAILED( _InitDevice() ) )
  { _CleanupDevice();
    return NULL;
  }
  return g_video.hwnd;
}

//--------------------------------------------------------------------------------------
// Uninitializes everything.
//--------------------------------------------------------------------------------------
unsigned int Video_Display_Release()
{ _CleanupDevice();

  if( g_video.frame_source )
  { Asynq_Unref( g_video.frame_source );
    g_video.frame_source = NULL;
  }
  return S_OK;
}

//--------------------------------------------------------------------------------------
// Called every time the application receives a message
//--------------------------------------------------------------------------------------
void
Video_Display_GUI_On_ID_COMMAND_VIDEODISPLAY(void)
{ HWND h = g_video.hwnd;
  if( !IsWindowVisible(h) || IsIconic(h) )
    ShowWindow( h, SW_SHOWNORMAL );
  else
    ShowWindow( h, SW_HIDE );
  UpdateWindow( h );
}

LRESULT
Video_Display_On_Sizing (  WPARAM wParam, LPARAM lParam )
{ RECT *rect = (RECT*) lParam;
  float aspect = g_video.aspect;  // want to maintain this width/height ratio
  LONG  width  = rect->right   - rect->left,
        height = rect->bottom  - rect->top;
  //width and height
  switch( wParam )
  {
    case WMSZ_BOTTOM:
    case WMSZ_TOP:
      width = (LONG) (aspect*height);
      break;
    case WMSZ_LEFT:
    case WMSZ_RIGHT:
      height = (LONG) (width/aspect);
      break;
    case WMSZ_BOTTOMLEFT:
    case WMSZ_BOTTOMRIGHT:
    case WMSZ_TOPLEFT:
    case WMSZ_TOPRIGHT:
      if(width>height)
        height = (LONG) (width/aspect);
      else
        width = (LONG) (aspect*height);
      break;
    default:
      error("Wierd wParam\r\n");
  }
  //anchor point - top or bottom
  switch( wParam )
  {
    case WMSZ_BOTTOM:      // use top left as anchor
    case WMSZ_RIGHT:
    case WMSZ_BOTTOMRIGHT:
      rect->right  = rect->left + width;
      rect->bottom = rect->top  + height;
      break;
    case WMSZ_TOP:         // use bottom right as anchor
    case WMSZ_LEFT:
    case WMSZ_TOPLEFT:
      rect->left = rect->right  - width;
      rect->top  = rect->bottom - height;
      break;               // use top right as anchor
    case WMSZ_BOTTOMLEFT:
      rect->left = rect->right  - width;
      rect->bottom = rect->top  + height;
      break;
    case WMSZ_TOPRIGHT:    // use bottom left as anchor
      rect->right  = rect->left + width;
      rect->top  = rect->bottom - height;
      break;
    default:
      error("Wierd wParam\r\n");
  }
  return TRUE;
}

LRESULT CALLBACK Video_Display_WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{   int wmId, wmEvent;
    PAINTSTRUCT ps;
    HDC hdc;

    switch( message )
    {
        case WM_PAINT:
            hdc = BeginPaint( hWnd, &ps );
            EndPaint( hWnd, &ps );
            break;

        case WM_DESTROY:
            PostQuitMessage( 0 );
            break;
        
        case WM_SIZE:
            //{ INT width  = LOWORD(lParam),
            //      height = HIWORD(lParam);
            //  g_video.swap_chain->ResizeBuffers(2, width, height, 
            //                                    DXGI_FORMAT_R8G8B8A8_UNORM, 
            //                                    DXGI_SWAP_CHAIN_FLAG_GDI_COMPATIBLE);
            //  _setup_viewport(width,height);
            //}
            break;
         
        case WM_SIZING:
            return Video_Display_On_Sizing(wParam, lParam );
            break; 
            
        case WM_COMMAND:
		        wmId    = LOWORD(wParam);
		        wmEvent = HIWORD(wParam);
		        // Parse the menu selections:
		        switch (wmId)
		        {
		        case ID_COMMAND_VIDEODISPLAY:
              Video_Display_GUI_On_ID_COMMAND_VIDEODISPLAY();
              break;
            case IDM_EXIT:
			        DestroyWindow(g_video.hwnd);
			        break;
		        default:
			        return DefWindowProc(hWnd, message, wParam, lParam);
		        }
		        break;

        default:
            return DefWindowProc( hWnd, message, wParam, lParam );
    }

    return 0;
}


//--------------------------------------------------------------------------------------
// Render a frame
//--------------------------------------------------------------------------------------

void Video_Display_Render_One_Frame()
{   TicTocTimer clock = tic();
    static Frame                  *frm = NULL;
    static Frame_Descriptor       last;
    static float          wait_time_ms = 50.0,//1000.0/1.0,
                efficiency_accumulator = 0.0, 
                      efficiency_count = 0.0,
                        efficiency_hit = 0.0;
    asynq               *q = g_video.frame_source;
    void              *src = NULL; 
    Frame_Descriptor *desc = NULL;
    static vector_size_t *vdim = NULL;
    Frame_Interface *fint = NULL;
    
    if( !vdim )
      vdim = vector_size_t_alloc(2);
    
    if( q )
    { // create the source buffer
      if(!frm)
      { frm  = (Frame*) Asynq_Token_Buffer_Alloc( q );
        Frame_From_Bytes( frm, &src, &desc );
      }
      
      if( Asynq_Peek_Timed(q, frm, (DWORD) wait_time_ms ) )
      { int i=3;
        Frame_From_Bytes( frm, &src, &desc );
        if( desc->is_change )
        { RECT *rect = NULL;
          Guarded_Assert( desc->is_change == 1 );
          memcpy(&last,desc,sizeof(Frame_Descriptor));
          //Resize window and buffers
          fint = Frame_Descriptor_Get_Interface(desc);
          fint->get_dimensions(desc, vdim);
          { size_t w = vdim->contents[0], 
                   h = vdim->contents[1];
            DXGI_MODE_DESC mode;
            mode.Width = w;
            mode.Height = h;
            mode.RefreshRate.Numerator = 60;
            mode.RefreshRate.Denominator = 1;
            mode.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            mode.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
            mode.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
            g_video.swap_chain->ResizeTarget(&mode);
            _invalidate_objects();
            Guarded_Assert(SUCCEEDED( g_video.swap_chain->ResizeBuffers(2, w, h,
                                              DXGI_FORMAT_R8G8B8A8_UNORM, 
                                              DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH)));
            _refresh_objects(w,h);
          }
        }
        while(i--)
          _copy_data_to_texture2d_ex( g_video.active_texture[i], src, &last, i );
          
        efficiency_accumulator++;
        efficiency_hit = 1.0;
      }
      if( efficiency_hit > 0.5 )
        efficiency_count++;
    }
    // TODO: Frame rate govenor
    
    //
    // Clear the back buffer
    //
    float ClearColor[4] = { 0.0f, 0.125f, 0.3f, 1.0f }; // red, green, blue, alpha
    g_video.device->ClearRenderTargetView( g_video.render_target_view, ClearColor );

    //
    // Update variables that change once per frame
    //
    //g_pWorldVariable->SetMatrix( ( float* )&g_World );
    //g_pMeshColorVariable->SetFloatVector( ( float* )g_vMeshColor );

    //
    // Render
    //
    D3D10_TECHNIQUE_DESC techDesc;
    g_video.technique->GetDesc( &techDesc );
    for( UINT p = 0; p < techDesc.Passes; ++p )
    {
        Guarded_Assert(SUCCEEDED(  g_video.technique->GetPassByIndex( p )->Apply( 0 ) ));
        g_video.device->DrawIndexed( 4, 0, 0 );
    }

    //
    // Present our back buffer to our front buffer
    //
    Guarded_Assert(SUCCEEDED(  g_video.swap_chain->Present( 0, 0 ) ));
    //// XXX: dx10 swap chain has it's own performance metrics
    //double dt = toc(&clock);
    //debug("FPS: %5.1f Efficiency: %g\r\n",1.0/dt, efficiency_accumulator/efficiency_count);
}

//--------------------------------------------------------------------------------------
// Connect frame source and reconfigure textures
//--------------------------------------------------------------------------------------
void Video_Display_Connect_Device( Device *source, size_t data_channel )
{ asynq *q = NULL;
  DeviceTask *task = source->task;
  Guarded_Assert( task );                            // source task must exist
  Guarded_Assert( task->out );                       // source channel must exist
  Guarded_Assert( task->out->nelem > data_channel ); // channel index must be valid

  q = Asynq_Ref( task->out->contents[data_channel] );// bind the queue  
  if( g_video.frame_source )                         // if there's an old queue, unref it first
    Asynq_Unref( g_video.frame_source );  
  g_video.frame_source = q;
  
  // TODO: reset texture resources for data format  
}
