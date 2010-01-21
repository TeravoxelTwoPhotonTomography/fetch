#include "stdafx.h"
#include "renderer-video-frame.h"
#include <d3d10.h>
#include <d3dx10.h>

typedef i16 TPixel;
#define VIDEO_FRAME_TEXTURE_FORMAT DXGI_FORMAT_R16_SNORM

Video_Frame_Resource*
Video_Frame_Resource_Alloc(void)
{ Video_Frame_Resource* self = (Video_Frame_Resource*) Guarded_Calloc( sizeof(Video_Frame_Resource), 1, "Video_Frame_Resource_Alloc");
  self->buf = vector_u8_alloc(1024*1024);
  return self;
}

void
Video_Frame_Resource_Free( Video_Frame_Resource *self )
{ if(self)
  { //ensure detached
    if(self->tex) // then the others got attached too
      Video_Frame_Resource_Detach(self);
    if(self->buf)
      vector_u8_free( self->buf );
    free(self);
  }
}

inline void 
_create_texture(ID3D10Device *device, ID3D10Texture3D **pptex, UINT width, UINT height, UINT nchan )
// Based on: ms-help://MS.VSCC.v90/MS.VSIPCC.v90/MS.Windows_Graphics.August.2009.1033/Windows_Graphics/d3d10_graphics_programming_guide_resources_creating_textures.htm#Creating_Empty_Textures
{ D3D10_TEXTURE3D_DESC desc;
  ZeroMemory( &desc, sizeof(desc) );

  desc.Width                         = width;
  desc.Height                        = height;
  desc.Depth                         = nchan;
  desc.MipLevels                     = 1;
  desc.Format                        = VIDEO_FRAME_TEXTURE_FORMAT;
  desc.Usage                         = D3D10_USAGE_DYNAMIC;
  desc.CPUAccessFlags                = D3D10_CPU_ACCESS_WRITE;
  desc.BindFlags                     = D3D10_BIND_SHADER_RESOURCE;
  
  Guarded_Assert( width  > 0 );
  Guarded_Assert( height > 0 );
  Guarded_Assert( nchan  > 0 );
  Guarded_Assert(SUCCEEDED( 
    device->CreateTexture3D( &desc, NULL, pptex ) 
    ));
}

inline void
_bind_texture_to_shader_variable(ID3D10Device *device, Video_Frame_Resource *self )
// Create the resource view for textures and bind it to the shader varable
{ D3D10_SHADER_RESOURCE_VIEW_DESC srvDesc;
  D3D10_RESOURCE_DIMENSION        type;
  D3D10_TEXTURE3D_DESC            desc;
  ID3D10Texture3D                *tex = self->tex;
  
  tex->GetType( &type );
  Guarded_Assert( type == D3D10_RESOURCE_DIMENSION_TEXTURE3D );
  tex->GetDesc( &desc );

  srvDesc.Format                    = desc.Format;
  srvDesc.ViewDimension             = D3D10_SRV_DIMENSION_TEXTURE3D;
  srvDesc.Texture3D.MipLevels       = desc.MipLevels;
  srvDesc.Texture3D.MostDetailedMip = desc.MipLevels - 1;

  Guarded_Assert(SUCCEEDED( 
    device->CreateShaderResourceView( tex,             // Create the view
                                      &srvDesc, 
                                      &self->srv ) ));
  Guarded_Assert(SUCCEEDED(
    self->shader_variable->SetResource( self->srv ) ));                  // Bind
}

void
Video_Frame_Resource_Attach ( Video_Frame_Resource *self,
                              UINT                 width,    // width  of frame in samples
                              UINT                height,    // height of frame in samples
                              UINT                 nchan,    // the number of channels.  Each channel gets its own colormap.
                              ID3D10Effect       *effect,    // bind to this effect
                              const char           *name )   // bind to this variable in the effect
{ ID3D10Device *device = NULL;
  Guarded_Assert(SUCCEEDED( effect->GetDevice(&device) ));
  
  self->shader_variable = effect->GetVariableByName(name)->AsShaderResource();
  _create_texture( device, &self->tex, width, height, nchan );
  _bind_texture_to_shader_variable( device, self );
  device->Release();
  
  self->nchan  = nchan;
  self->nlines = height;
  self->stride = width * sizeof(TPixel);
  vector_u8_request(self->buf, nchan * width * height * sizeof(TPixel) );  
}

void
Video_Frame_Resource_Detach ( Video_Frame_Resource *self )
{ if( self->srv) self->srv->Release();
  if( self->tex) self->tex->Release();
  self->srv = NULL;
  self->tex = NULL;
}

void
Video_Frame_Resource_Commit ( Video_Frame_Resource *self )
/* Copies internal buffer into texture resource */
{ D3D10_MAPPED_TEXTURE3D  dst;
  ID3D10Texture3D        *tex = self->tex;
  void                   *src = self->buf->contents;
  Guarded_Assert(SUCCEEDED( 
      tex->Map( D3D10CalcSubresource(0, 0, 1), D3D10_MAP_WRITE_DISCARD, 0, &dst ) ));  
  Copy_Planes_By_Lines( dst.pData, dst.RowPitch, dst.DepthPitch,
                        src, self->stride, self->stride*self->nlines,
                        self->nlines, self->nchan );
  tex->Unmap( D3D10CalcSubresource(0, 0, 1) );  
}

void
Video_Frame_From_Frame_Descriptor ( Video_Frame_Resource *self, void *src,
                                    Frame_Descriptor *desc )
{ Frame_Interface *f = Frame_Descriptor_Get_Interface( desc );
  size_t ichan;
  
  vector_u8_request( self->buf, f->get_nbytes(desc) );
  for(ichan=0; ichan<self->nchan; ichan++ )
    f->copy_channel(desc, self->buf->contents + ichan*self->stride*self->nlines, self->stride, src, ichan );
}

void
Video_Frame_From_Raw( Video_Frame_Resource *self, void *src,
                      size_t width, size_t height, size_t nchan )
{ size_t nbytes = width*height*nchan*sizeof(TPixel);
  vector_u8_request(self->buf, nbytes);
  memcpy(self->buf->contents, src, nbytes);
}