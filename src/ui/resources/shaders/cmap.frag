#version 130
uniform sampler3D plane; // image data
uniform sampler2D cmap;  // colormap
uniform sampler2D sctrl; // control points indexing colormap
uniform sampler2D tctrl; //

uniform float nchan;     // number of channels to colormap
uniform float fill;      // fraction that a channel's color contributes to a pixel
uniform float gain;      // applied to all channels - modifies indexing between ctrl pts
uniform float bias;      // applied to all channels 
uniform float gamma;     // applied to all channels

uniform int   show_mode; // bypasses mixing and shows one of the other textures instead; for debugging

/*
   Takes an image with color channels arrayed as succesive scalar planes and
   blends according to cmap.  Cmap is a NxM RGBA image where N is the number
   of intensity levels spanned by the different planes and M is the number
   of colors.
   */
out vec4 FragColor;
in vec2 TexCoord0;
void main(void)
{
  
  vec3 uvw = vec3(0.0,0.0,0.0);
  uvw.st = TexCoord0; //gl_TexCoord[0].st; 

  switch(show_mode)
  { 
  case 1:
    { FragColor = texture(cmap,uvw.st);     //directly output cmap; ignore image
      return;
    }
  case 2:
    { vec4 c = texture(sctrl,uvw.st);
      FragColor = vec4(c.x,c.x,c.x,1.0);
    }
    return;
  case 3:
    { vec4 c = texture(tctrl,uvw.st);
      c.x = pow(c.x,gamma);                      //gamma
      FragColor = vec4(c.x,c.x,c.x,1.0);
    }
    return;
  default:
    if( (nchan-1.0)<1e-2 ) // nchan==1
    { 
      vec4 v    = texture(plane,uvw);             //luma
      v.x       = gain*v.x + bias;                //adjust instensity
      v.x       = texture(tctrl,vec2(v.x,0.0)).x; //use the lookup from the first channel      
      v.x       = pow(v.x,gamma);                 //gamma
      vec4 c    = texture(cmap,vec2(1.0,v.x));    //cmap (sample along vertical)      
      //vec4 c    = vec4(v.x,v.x,v.x,1.0);          //cmap black and white (no texture)
      FragColor = vec4(c.r,c.g,c.b,1.0);
      return;
    } else
    {
      float a = 1.0/(nchan-1.0);  // e.g. 2 channels -> a=1.0; 3 channels -> a=0.5
      vec4 c = vec4(0.0,0.0,0.0,1.0);
      for(float i=0.0;i<nchan;++i)
      { vec4 v,s,t;
        vec2 p;
        uvw.z = a*i;                              // addresses the channel ( 2ch->(0,1); 3ch->(0,0.5,1) ) 
        v = texture(plane,uvw);                   // luma                                                 
        v.x = pow(v.x,gamma);                     // gamma adjust                                         
        { vec2 mu = vec2(v.x,uvw.z);
          p.x = texture(sctrl,mu).x;
          p.y = texture(tctrl,mu).x;
        }
        c += fill*texture(cmap,p);
      }
      c.w=1.0;
      FragColor = c;
      return;
    }
  }
}

