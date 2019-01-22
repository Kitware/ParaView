// clang-format off
//VTK::System::Dec
//VTK::Output::Dec
// clang-format on

uniform vec3 color;
uniform int scalarVisibility;

varying vec3 vertexColorVSOutput;

void main(void)
{
  if (scalarVisibility != 0)
    gl_FragData[0] = vec4(vertexColorVSOutput, 1.);
  else
    gl_FragData[0] = vec4(color, 1.);
}
