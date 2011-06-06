#version 130
varying vec2 TexCoord0;

void main(void)
{ 
  TexCoord0 = gl_MultiTexCoord0.st;
  gl_Position = ftransform(); // dep. in 130: have to use you're own matrix stack?
}
