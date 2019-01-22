//VTK::System::Dec
//VTK::Output::Dec

varying vec2 tcoordVC;

uniform sampler2D source;

void main(void)
{
  gl_FragData[0] = texture2D(source, tcoordVC);
}
