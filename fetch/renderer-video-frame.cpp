#include "stdafx.h"
#include "renderer-video-frame.h"
#include <d3d10.h>
#include <d3dx10.h>
#include "util/util-dx10.h"
#include "util/util-image.h"

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
_create_texture(ID3D10Device *device, ID3D10Texture3D **pptex, Basic_Type_ID rtti, UINT width, UINT height, UINT nchan )
// Based on: ms-help://MS.VSCC.v90/MS.VSIPCC.v90/MS.Windows_Graphics.August.2009.1033/Windows_Graphics/d3d10_graphics_programming_guide_resources_creating_textures.htm#Creating_Empty_Textures
{ D3D10_TEXTURE3D_DESC desc;
  ZeroMemory( &desc, sizeof(desc) );

  desc.Width                         = width;
  desc.Height                        = height;
  desc.Depth                         = nchan;
  desc.MipLevels                     = 1;
  desc.Format                        = g_dx10_tex_type_map[rtti];
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
                              Basic_Type_ID         type,    // the pixel type
                              UINT                 width,    // width  of frame in samples
                              UINT                height,    // height of frame in samples
                              UINT                 nchan,    // the number of channels.  Each channel gets its own colormap.
                              ID3D10Effect       *effect,    // bind to this effect
                              const char           *name )   // bind to this variable in the effect
{ ID3D10Device *device = NULL;
  size_t Bpp = g_type_attributes[type].bytes;
  Guarded_Assert(SUCCEEDED( effect->GetDevice(&device) ));
  
  self->shader_variable = effect->GetVariableByName(name)->AsShaderResource();
  _create_texture( device, &self->tex, type, width, height, nchan );
  _bind_texture_to_shader_variable( device, self );
  device->Release();
  
  self->type   = type;
  self->nchan  = nchan;
  self->nlines = height;
  self->stride = width * Bpp;
  vector_u8_request(self->buf, nchan * width * height * Bpp );  
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
  { size_t src_pitch[4],
           dst_pitch[4] = {dst.DepthPitch*self->nchan,                      // - in destination, each channel is a plane
                           dst.DepthPitch,
                           dst.RowPitch,
                           1};                                              // - treat each line like a u8 string.
    size_t shape[3]     = {self->nlines, self->nchan, self->stride};        // - shape of source region to copy
    Compute_Pitch( src_pitch, self->nlines, self->nchan, self->stride, 1);  // - treat each line like a u8 string.
    imCopyTranspose<u8,u8>( (u8*) dst.pData, dst_pitch, 
                            (u8*) src,       src_pitch, shape, 0, 1 );      // - on copy, switch lines and nchan dimensions
    //{ FILE *fp = fopen("src.raw","wb");
    //  fwrite(src, 1, src_pitch[0], fp );
    //  fclose(fp);
    //}
  }

  //{ FILE *fp = fopen("tex.raw","wb");
  //  fwrite(dst.pData, 1, dst.DepthPitch * self->nchan, fp );
  //  fclose(fp);
  //}
  tex->Unmap( D3D10CalcSubresource(0, 0, 1) );  
}

void
Video_Frame_From_Frame( Video_Frame_Resource *self, Frame *src )
{ size_t ichan;

  src->dump("src.raw");
  
  vector_u8_request( self->buf, src->size_bytes() );
  for(ichan=0; ichan<self->nchan; ichan++ )
    src->copy_channel(self->buf->contents + ichan*self->stride*self->nlines, 
                      self->stride,
                      ichan );
}

void
Video_Frame_From_Raw( Video_Frame_Resource *self, void *src,
                      size_t Bpp, size_t width, size_t height, size_t nchan )
{ size_t nbytes = width*height*nchan*Bpp;
  vector_u8_request(self->buf, nbytes);
  memcpy(self->buf->contents, src, nbytes);
}

#define _AUTOLEVEL_HIST(TPixel) {\
    TPixel *beg = (TPixel*)(self->buf->contents + ichan * channel_stride),\
           *end = (TPixel*)(self->buf->contents + (ichan+1) * channel_stride);\
    counts = (float) (end - beg);\
    while(end-- > beg )\
      hist[ (int)((*end - low)*scale) ]++;\
  } break
  
void
Video_Frame_Autolevel( Video_Frame_Resource *self, int ichan, float thresh, float *min, float *max )                                // WARNING: FIXME: assumes internal buffer has a specific type (i16)
{ int bits      = g_type_attributes[ self->type ].bits; // eg: 16
  int issigned  = g_type_attributes[ self->type ].is_signed;
  u16 hist[256];
  int i = 0;
  size_t channel_stride =  self->stride * self->nlines;  
  float  norm = (float) (1 << bits),
        scale = 256.f/norm,
          low = (float) (issigned ? -(1<<(bits-1)) : 0),
          acc,
       counts,
          lim;
  
  memset( hist,0,sizeof(hist) );  // compute histogram
  switch( self->type )
  { case id_u8:  _AUTOLEVEL_HIST(u8 );
    case id_u16: _AUTOLEVEL_HIST(u16);
    case id_u32: _AUTOLEVEL_HIST(u32);
    case id_u64: _AUTOLEVEL_HIST(u64);
    case id_i8:  _AUTOLEVEL_HIST(i8 );
    case id_i16: _AUTOLEVEL_HIST(i16);
    case id_i32: _AUTOLEVEL_HIST(i32);
    case id_i64: _AUTOLEVEL_HIST(i64);
    case id_f32: _AUTOLEVEL_HIST(f32);
    case id_f64: _AUTOLEVEL_HIST(f64);
    default:
      error("Unrecognized pixel type in Video_Frame_Autolevel.\r\n");
  }
  lim = thresh * counts; // thresh is a percentage (e.g. 0.05 ).  Find where the integrated histogram has area more than lim.
  acc = 0.0;
  while( i++ < 256 )     // integrate till limit exceeded
  { acc += hist[i];
    if( acc > lim )
      break;
  }
  *min = i/256.0f;
  
  lim = (1.0f-thresh) * counts;
  acc = counts;
  i = 256;
  while( i-- > 0 )       // integrate till limit exceeded 
  { acc -= hist[i];
    if( acc < lim )
      break;
  }
  *max = i/256.0f;
  return;
}
