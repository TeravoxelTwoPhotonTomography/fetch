//
// TODO
// [ ] - set shader scalar function
// [ ] - automatically change number of channels on a description change
// [ ] - seperate texture related code similar to colormaps
// [ ] - make a util-dx10.[ch] and move some code out
//
// [ ] - autolevel
//
// NOTES
// -----
// Texture arrays won't work because dynamic resource can only have a single subresource for some reason.
//

#include "stdafx.h"
#include <d3d10.h>
#include <d3dx10.h>
#include "util-dx10.h"
#include "window-video.h"
#include <stdlib.h> // for rand - for testing - remove when done
#include "asynq.h"
#include "frame.h"
#include "renderer-colormap.h"
#include "renderer-video-frame.h"

#define VIDEO_WINDOW_TEXTURE_RESOURCE_NAME  "tx"
#define VIDEO_WINDOW_COLORMAP_RESOURCE_NAME "cmap"
#define VIDEO_WINDOW_NUM_CHAN_RESOURCE_NAME "nchan"
#define VIDEO_WINDOW_PATH_TO_SHADER         "shader.fx"
#define VIDEO_WINDOW_SHADER_TECHNIQUE_NAME  "Render"

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
  
  Video_Frame_Resource               *vframe;
  
  Colormap_Resource                  *cmaps;
  float                              *mins;                               // array[nchan]: range from 0.0 to 1.0 - used for colormap gerneation
  float                              *maxs;                               // array[nchan]: range from 0.0 to 1.0 - used for colormap generation  
  
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
                              NULL,\
                              NULL,\
                              NULL,\
                              NULL,\
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
    RECT rc = { 0, 0, 256, 256 };
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
  Guarded_Assert(SUCCEEDED( hr = g_video.swap_chain->GetBuffer( 0, __uuidof( ID3D10Texture2D ), ( LPVOID* )&pBuffer ) )); // get the display buffer   
  Guarded_Assert(SUCCEEDED( hr = g_video.device->CreateRenderTargetView( pBuffer, NULL, &g_video.render_target_view ) )); // ... get a view to the buffer
  pBuffer->Release();                                                                                                     // ... (done with the buffer itself)
  g_video.device->OMSetRenderTargets( 1, &g_video.render_target_view, NULL );                                             // ... and bind to as a render output
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

inline void _load_colormaps(UINT nchan)
{ int nrows = 1<<10;
 
  Guarded_Assert(g_video.mins);
  Guarded_Assert(g_video.maxs);

  Colormap_Resource_Attach( g_video.cmaps, nrows, nchan, g_video.effect, VIDEO_WINDOW_COLORMAP_RESOURCE_NAME );
  Colormap_Autosetup( g_video.cmaps, g_video.mins, g_video.maxs );
  Colormap_Resource_Commit( g_video.cmaps );
}

inline void
_load_shader(const char* path, const char* technique)
{ HRESULT hr = S_OK;
  DWORD dwShaderFlags = D3D10_SHADER_ENABLE_STRICTNESS;
  ID3D10Blob *shader_errors = NULL;
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
                                   &shader_errors,
                                   NULL );
  if( FAILED( hr ) )
  { debug("%s\r\n", (char*)shader_errors->GetBufferPointer() );
    error( "Could not compile the FX file.  Perhaps it could not be located or there was a compilation error.");
  }
  

  // Obtain the technique and get references to variables
  g_video.technique                         = g_video.effect->GetTechniqueByName( technique );  
  Guarded_Assert( g_video.technique );  
  if( shader_errors )
    shader_errors->Release();
}
      
HRESULT _InitDevice()
{ HRESULT hr = S_OK;
  RECT rc;
  GetClientRect( g_video.hwnd, &rc );
  UINT width  = 512; //rc.right - rc.left;               // defaults used for startup
  UINT height = 512; //rc.bottom - rc.top;
  UINT nchan  = 3;             
  Basic_Type_ID type = id_i16;
       
  _create_device_and_swap_chain(width, height);
  _setup_viewport( width, height );
  _load_shader(VIDEO_WINDOW_PATH_TO_SHADER, VIDEO_WINDOW_SHADER_TECHNIQUE_NAME);
  _setup_geometry();
  
  { g_video.vframe = Video_Frame_Resource_Alloc();
    Video_Frame_Resource_Attach( g_video.vframe,
                                 type, width, height, nchan,
                                 g_video.effect, VIDEO_WINDOW_TEXTURE_RESOURCE_NAME );
    
    // Initialize the texture with test data
    { typedef i16 T;
      size_t src_nelem = width*height*nchan;      // create the source buffer
      T *src = (T*)calloc(src_nelem,sizeof(T));
      unsigned int i,j,k;
      for(k=0;k<nchan;k++)
        for(i=0;i<height;i++)
          for(j=0;j<width;j++)
            src[j + width * i + k * height * width] = (T) j + width * i + rand()/(1<<4);
      Video_Frame_From_Raw( g_video.vframe, src, sizeof(T), width, height, nchan );
      free(src);        
    }
    Video_Frame_Resource_Commit( g_video.vframe );
  }                                          
  if( FAILED( hr ) )
      return hr;

  g_video.mins = (float*) Guarded_Malloc( sizeof(float)*nchan, "window-video:_InitDevice():alloc mins" );
  g_video.maxs = (float*) Guarded_Malloc( sizeof(float)*nchan, "window-video:_InitDevice():alloc maxs" );
  { UINT i = nchan;
    while(i--)
    { g_video.mins[i] = 0.0;
      g_video.maxs[i] = 1.0;
    }
  }
  
  g_video.cmaps = Colormap_Resource_Alloc();  
  
  _load_colormaps(nchan);
  dx10_effect_variable_set_f32(g_video.effect, VIDEO_WINDOW_NUM_CHAN_RESOURCE_NAME, (f32) nchan);
  
  debug("%f\r\n", dx10_effect_variable_get_f32( g_video.effect, "nchan" ));
    
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
    
    if( g_video.vframe )                Video_Frame_Resource_Free( g_video.vframe ); g_video.vframe = NULL;
        
    if( g_video.cmaps )                 Colormap_Resource_Free  ( g_video.cmaps );    
    
    if( g_video.effect )                g_video.effect->Release();
    if( g_video.render_target_view )    g_video.render_target_view->Release();
    if( g_video.swap_chain )            g_video.swap_chain->Release();
    if( g_video.device )                g_video.device->Release();
    
    if(g_video.mins) free( g_video.mins ); g_video.mins = NULL;
    if(g_video.maxs) free( g_video.maxs ); g_video.maxs = NULL;
}

//--------------------------------------------------------------------------------------
// Set up the device objects (setup after a resize)
//--------------------------------------------------------------------------------------
void _refresh_objects(Basic_Type_ID type, UINT width, UINT height, UINT nchan)
{ int i = 3;
  HRESULT hr;
  // Create a render target view
  { ID3D10Texture2D* pBuffer;
  
    Guarded_Assert(SUCCEEDED( hr = g_video.swap_chain->GetBuffer( 0, __uuidof( ID3D10Texture2D ), ( LPVOID* )&pBuffer ) ));    
    Guarded_Assert(SUCCEEDED( hr = g_video.device->CreateRenderTargetView( pBuffer, NULL, &g_video.render_target_view ) ));
    pBuffer->Release();
    g_video.device->OMSetRenderTargets( 1, &g_video.render_target_view, NULL );
  }
  _setup_viewport( width, height );
  _load_shader(VIDEO_WINDOW_PATH_TO_SHADER, VIDEO_WINDOW_SHADER_TECHNIQUE_NAME);
  Video_Frame_Resource_Attach( g_video.vframe, type, width, height, nchan, g_video.effect, VIDEO_WINDOW_TEXTURE_RESOURCE_NAME );
  _load_colormaps(nchan);
  dx10_effect_variable_set_f32(g_video.effect, VIDEO_WINDOW_NUM_CHAN_RESOURCE_NAME, (f32) nchan);
}

//--------------------------------------------------------------------------------------
// Clean up the device objects (prep for a resize)
//--------------------------------------------------------------------------------------
void _invalidate_objects(void)
{ int cnt = 0;
  cnt = g_video.render_target_view->Release();
  cnt = g_video.effect->Release();

  Video_Frame_Resource_Detach( g_video.vframe );
  Colormap_Resource_Detach   ( g_video.cmaps  );
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
      warning("Video_Display_On_Sizing: Wierd wParam\r\n");
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
      warning("Video_Display_On_Sizing: Wierd wParam\r\n");
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

        case WM_KEYDOWN:
            if( (lParam & 0xf) == 1 ) // if repeat count is one
            { switch( wParam )
              { case 0x30: // "0" key
                case 0x31: // "1" key
                case 0x32: // "2" key
                { size_t ichan = wParam - 0x30;                  
                  if( ichan < g_video.vframe->nchan )
                  { Video_Frame_Autolevel( g_video.vframe, ichan, /*0.05f*/0.0, g_video.mins+ichan, g_video.maxs+ichan );
                    debug("Autolevel channel %d - [%4.3f, %4.3f]\r\n", ichan, g_video.mins[ichan], g_video.maxs[ichan]);
                    Colormap_Autosetup( g_video.cmaps, g_video.mins, g_video.maxs );
                    Colormap_Resource_Commit( g_video.cmaps );
                  }
                  break;
                }                  
                default:
                  break;
              }            
            }
            
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
    static FrmFmt              lastfmt;
    static float          wait_time_ms = 1000.0f/60.0f,
                efficiency_accumulator = 0.0f,
                      efficiency_count = 0.0f;

    asynq                      *q = g_video.frame_source;
    static int  last_change_token = 0;
    static size_t           nchan = 0;
    
    if( q )
    { // create the source buffer
    
      if(!frm)
      { frm  = (Frame*) Asynq_Token_Buffer_Alloc( q );
      }
      
      if( Asynq_Peek_Timed(q, frm, (DWORD) wait_time_ms ) )
      { int i;   
        if( !frm->is_equivalent(&lastfmt) )               // RESIZE!
        { RECT *rect = NULL;
          frm->format(&lastfmt);
          //Resize window and buffers
          { size_t           w = frm->width,
                             h = frm->height;
            Basic_Type_ID type = frm->rtti;
            nchan = frm->nchan;
                   
            DXGI_MODE_DESC mode;
            mode.Width                   = 2*w;
            mode.Height                  = 2*h;
            mode.RefreshRate.Numerator   = 60;
            mode.RefreshRate.Denominator = 1;
            mode.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
            mode.ScanlineOrdering        = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
            mode.Scaling                 = DXGI_MODE_SCALING_UNSPECIFIED;
            
            g_video.swap_chain->ResizeTarget(&mode);
            _invalidate_objects();
            Guarded_Assert(SUCCEEDED( g_video.swap_chain->ResizeBuffers(2, w, h,
                                              DXGI_FORMAT_R8G8B8A8_UNORM, 
                                              DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH)));
            _refresh_objects(type,w,h,nchan);
            g_video.aspect = w/((float)h);
          }
        } // end if change
        i = nchan;
        Video_Frame_From_Frame( g_video.vframe, frm);
        Video_Frame_Resource_Commit( g_video.vframe );
          
        efficiency_accumulator++;
      } // end if try peek
      efficiency_count++;
    } // end if q (if anything is connected)
    // [ ] TODO: Frame rate govenor
    
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
  
  // TODO: reset texture resources for data format  ( currently this is done in the render function...perhaps there's a reason to move it? )
}
