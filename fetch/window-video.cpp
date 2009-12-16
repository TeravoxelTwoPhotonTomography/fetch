#include "stdafx.h"
#include <d3d10.h>
#include <d3dx10.h>
#include "window-video.h"
#include <stdlib.h> // for rand - for testing - remove when done
#include "asynq.h"
#include "frame.h"

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
  
  asynq                              *frame_source;
  asynq                              *frame_desc_source;
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
                              NULL,\
                              NULL}

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
    RECT rc = { 0, 0, 640, 480 };
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
  UINT msaa_quality = 0, msaa_levels = 1;
  
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
  if( g_video.effect->GetVariableByName( "tx" )->IsValid() )
    debug("huh\n");
  g_video.active_texture_shader_resource[0] = g_video.effect->GetVariableByName( "txR" )->AsShaderResource();
  g_video.active_texture_shader_resource[1] = g_video.effect->GetVariableByName( "txG" )->AsShaderResource();
  g_video.active_texture_shader_resource[2] = g_video.effect->GetVariableByName( "txB" )->AsShaderResource();  
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
  desc.Width                         = 1024;
  desc.Height                        = 1024;
  desc.MipLevels = desc.ArraySize    = 1;
  desc.Format                        = format;
  desc.SampleDesc.Count              = 1;
  desc.Usage                         = D3D10_USAGE_DYNAMIC;
  desc.CPUAccessFlags                = D3D10_CPU_ACCESS_WRITE;
  desc.BindFlags                     = D3D10_BIND_SHADER_RESOURCE;
  
  while(i--)
  { if( g_video.active_texture[i] )
    { g_video.active_texture[i]->Release();
      g_video.active_texture[i] = NULL;
    }
    Guarded_Assert( !FAILED( g_video.device->CreateTexture2D( &desc, NULL, g_video.active_texture+i ) ));
  }
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
  { if( g_video.texture_resource_view[i] )
    { g_video.texture_resource_view[i]->Release();
      g_video.texture_resource_view[i] = NULL;
    }
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
  UINT width = rc.right - rc.left;
  UINT height = rc.bottom - rc.top;  
    
  _create_device_and_swap_chain(width, height);
  _setup_viewport( width, height );
  _load_shader(VIDEO_WINDOW_PATH_TO_SHADER, VIDEO_WINDOW_SHADER_TECHNIQUE_NAME);
  _setup_geometry();
                                  
  
  { _create_texture_i16( g_video.active_texture, 1024, 1024 );               
    _refresh_active_texture_shader_resource_view();
    
    // Initialize the texture with test data
    { typedef i16 T;
      size_t src_nelem = 1024*1024;      // create the source buffer
      u16 *src = (u16*)calloc(src_nelem,sizeof(T));
      int i,j;
      for(i=0;i<1024;i++)
        for(j=0;j<1024;j++)
          src[j + 1024 * i] = (T) j + 1024 * i + rand()/(1<<16);
      _copy_data_to_texture2d( g_video.active_texture[0], src, src_nelem*sizeof(T) );
      _copy_data_to_texture2d( g_video.active_texture[1], src, src_nelem*sizeof(T) );
      _copy_data_to_texture2d( g_video.active_texture[2], src, src_nelem*sizeof(T) );
      free(src);        
    }      
  }                                          
  if( FAILED( hr ) )
      return hr;
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
    }
    
    if( g_video.effect )                g_video.effect->Release();
    if( g_video.render_target_view )    g_video.render_target_view->Release();
    if( g_video.swap_chain )            g_video.swap_chain->Release();
    if( g_video.device )                g_video.device->Release();
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
    static u8* src = NULL; 
    static Frame_Descriptor *desc = NULL, *tmp = NULL;
    static asynq *qs[2]   = {g_video.frame_source, g_video.frame_desc_source};
    static void  *bufs[2];
    size_t src_nbytes = 1024*1024*sizeof(u16);
    static float wait_time_ms = 1000.0/10.0,
                efficiency_accumulator = 0.0, 
                efficiency_count = 0.0,
                efficiency_hit = 0.0;
 
      
    //{ int i,j;
    //  int low = 1024 * 0.1,
    //     high = 1024 * 0.9;
    //  for(i=low;i<high;i++)
    //    for(j=low;j<high;j++)
    //      src[j + 1024 * i] = (u8) j + 1024 * i + rand()/256;
    //}
    //_copy_data_to_texture2d( g_video.active_texture, src, src_nbytes );
    
    if( g_video.frame_source )
    { // create the source buffer
      if(!src)
      { src  = (u8*) Asynq_Token_Buffer_Alloc( qs[0] );
        desc = (Frame_Descriptor*) Asynq_Token_Buffer_Alloc( qs[1] );
        tmp  = (Frame_Descriptor*) Asynq_Token_Buffer_Alloc( qs[1] );
        bufs[0] = src;
        bufs[1] = tmp;
      }
      
      //if( Asynq_Peek_Timed( g_video.frame_source, src, (DWORD) wait_time_ms ) ){}
      if( Asynq_Slaved_Peek_Timed( qs, 2, 0, bufs, (DWORD) wait_time_ms ) )
      { int i=3;
        if( tmp->is_change == 1 )
        { memcpy(desc,tmp,sizeof(Frame_Descriptor));
        }
        while(i--)
          _copy_data_to_texture2d_ex( g_video.active_texture[i], src, desc, i );
        //while(i--)
        //  _copy_data_to_texture2d( g_video.active_texture[i], src + i * src_nbytes, src_nbytes );
        efficiency_accumulator++;
        efficiency_hit = 1.0;
      }
      if( efficiency_hit > 0.5 )
        efficiency_count++;
    }
    // Frame rate govenor
    if( efficiency_hit )
    { float mean = efficiency_accumulator/efficiency_count;
      wait_time_ms += -50.0f * (mean - 0.5f);
      wait_time_ms = MIN( wait_time_ms, 1000.0f );
      wait_time_ms = MAX( wait_time_ms, 0.0f );
    }
    
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
        g_video.technique->GetPassByIndex( p )->Apply( 0 );
        g_video.device->DrawIndexed( 4, 0, 0 );
    }

    //
    // Present our back buffer to our front buffer
    //
    g_video.swap_chain->Present( 0, 0 );
    //
    //double dt = toc(&clock);
    //debug("FPS: %5.1f Efficiency: %g\r\n",1.0/dt, efficiency_accumulator/efficiency_count);
}

//--------------------------------------------------------------------------------------
// Connect frame source and reconfigure textures
//--------------------------------------------------------------------------------------
void Video_Display_Connect_Device( Device *source, size_t data_channel, size_t desc_channel )
{ asynq *q = NULL;
  DeviceTask *task = source->task;
  Guarded_Assert( task );                            // source task must exist
  Guarded_Assert( task->out );                       // source channel must exist
  Guarded_Assert( task->out->nelem > data_channel ); // channel index must be valid
  Guarded_Assert( task->out->nelem > desc_channel ); // channel index must be valid

  q = Asynq_Ref( task->out->contents[data_channel] );// bind the queue  
  if( g_video.frame_source )                         // if there's an old queue, unref it first
    Asynq_Unref( g_video.frame_source );  
  g_video.frame_source = q;
  
  q = Asynq_Ref( task->out->contents[desc_channel] );// bind the queue  
  if( g_video.frame_desc_source )                    // if there's an old queue, unref it first
    Asynq_Unref( g_video.frame_desc_source );  
  g_video.frame_desc_source = q;
  
  // TODO: reset texture resources for data format  
}