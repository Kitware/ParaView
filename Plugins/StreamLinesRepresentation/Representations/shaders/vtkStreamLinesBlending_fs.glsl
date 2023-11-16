//VTK::System::Dec
//VTK::Output::Dec

varying vec2 tcoordVC;

uniform sampler2D prev;
uniform sampler2D prevDepth;
uniform sampler2D current;
uniform sampler2D currentDepth;
uniform float alpha;

void main(void)
{
  vec4 pc = texture2D(prev, tcoordVC);
  vec4 cc = texture2D(current, tcoordVC);
  gl_FragData[0] = pc + cc;
  gl_FragData[0].a *= alpha;
  float pd = texture2D(prevDepth, tcoordVC).x; // previous depth
  float cd = texture2D(currentDepth, tcoordVC).x; // current depth
  if (cd < 1.0)
  {
    gl_FragDepth = cd;
  }
  else
  {
    gl_FragDepth = pd;
  }
}
