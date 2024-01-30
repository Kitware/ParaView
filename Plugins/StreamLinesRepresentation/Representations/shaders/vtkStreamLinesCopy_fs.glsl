//VTK::System::Dec
//VTK::Output::Dec
//VTK::DepthPeeling::Dec
varying vec2 tcoordVC;

uniform sampler2D source;
uniform sampler2D depthSource;

void main(void)
{
  gl_FragDepth = texture2D(depthSource, tcoordVC).x;
  //VTK::DepthPeeling::PreColor
  gl_FragData[0] = texture2D(source, tcoordVC);
  if (gl_FragData[0].a <= 0.0)
    {
    discard;
    }
  //VTK::DepthPeeling::Impl
}
