uniform sampler3D plane;
uniform sampler2D cmap;
uniform float nchan;
uniform float fill;
uniform float gain;
uniform float bias;
uniform bool  show_cmap;

/*
   Takes an image with color channels arrayed as succesive scalar planes and
   blends according to cmap.  Cmap is a NxM RGBA image where N is the number
   of intensity levels spanned by the different planes and M is the number
   of colors.
   */

void main(void)
{
  vec3 uvw = gl_TexCoord[0].xyz;
  if(show_cmap)
  { gl_FragColor = texture2D(cmap,uvw.st);     //directly output cmap; ignore image
    return;
  }
        

  
  if( (nchan-1.0)<1e-2 ) // nchan==1
  { 
        vec4 v = texture3D(plane,uvw);  //luma
    v.x = gain*v.x - bias;                     //adjust instensity
        //vec4 c = texture2D(cmap,vec2(1.0,v.x));  //cmap (sample along vertical)
        //vec4 c = texture2D(cmap,uvw.st);         //directly output cmap; ignore image
    vec4 c = vec4(v.x,v.x,v.x,1.0);          //cmap black and white
        gl_FragColor = vec4(c.r,c.g,c.b,1.0);
    return;
  } else
  {
    float a = 1.0/(nchan-1.0);
        vec4 c = vec4(0.0,0.0,0.0,1.0);
        for(float i=0.0;i<nchan;++i)
    { vec4 v;
      uvw.z = a*i;
      v = texture3D(plane,uvw);
            c += fill*texture2D(cmap,vec2(uvw.z,gain*v.x-bias));
    }
    c.w=1.0;
    gl_FragColor = c;
        return;
  }
}

/***     ***
 *** OLD ***
 ***     ***/   
 /*
   // This blends planes above and below ichan
   // using different colors for the different planes.
uniform float ichan;
uniform float alpha;

void main(void)
{
  vec3 uvw = gl_TexCoord[0].xyz;
  vec4 c = vec4(0.0,0.0,0.0,1.0);
  float a = 1.0/nchan;
  //for(float i=0.0;i<nchan;i++)
  for(float i=ichan-10.0;i<ichan+10.0;i++)
  { vec4 v;
    uvw.z = a*i;
    v = texture3D(plane,uvw);
    c += alpha*texture2D(cmap,vec2((i-ichan+10.0)/20.0,v.x));
  }
  c.w=1.0;
  gl_FragColor = c;
}
*/
/*
 Trivial - solid color
 */
 /*
void main(void)
{
    gl_FragColor = vec4(1.0,1.0,0.0,1.0);
}
*/
