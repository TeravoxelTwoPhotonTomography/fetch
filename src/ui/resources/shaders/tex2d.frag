uniform sampler2D plane;
uniform sampler2D cmap;


/*
   Takes an image with color channels arrayed as succesive scalar planes and
   blends according to cmap.  Cmap is a NxM RGBA image where N is the number
   of intensity levels spanned by the different planes and M is the number
   of colors.

   gl_TexCoord[0] must get set up by vertex shader.
   */

void main(void)
{
  vec2 uv = gl_TexCoord[0].st;
  vec4 v = texture2D(plane,uv);   //luma
  vec4 c = texture2D(cmap,v.st);  //cmap
  gl_FragColor = vec4(c.r,c.y,c.z,1.0);
  return;
}

