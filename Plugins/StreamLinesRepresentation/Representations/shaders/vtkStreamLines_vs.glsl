//VTK::System::Dec
//VTK::Output::Dec

attribute vec4 vertexMC;
attribute vec3 scalarColor;

uniform mat4 MCDCMatrix;

varying vec3 vertexColorVSOutput;

void main(void)
{
  vertexColorVSOutput = scalarColor.rgb;
  gl_Position = MCDCMatrix * vertexMC;
}
