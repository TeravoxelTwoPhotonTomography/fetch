varying out vec2 TexCoord0;
void main(void)
{ 
  TexCoord0 = gl_MultiTexCoord0.st;
  gl_Position = ftransform();
}
