// clang-format off
//VTK::System::Dec
//VTK::Output::Dec
// clang-format on

in vec3 vertexColorVSOutput[];

out vec3 vertexColorGSOutput;

uniform vec2 lineWidthNVC;

layout(lines) in;
layout(triangle_strip, max_vertices = 4) out;

void main()
{
  // compute the lines direction
  vec2 normal = normalize(gl_in[1].gl_Position.xy / gl_in[1].gl_Position.w -
    gl_in[0].gl_Position.xy / gl_in[0].gl_Position.w);

  // rotate 90 degrees
  normal = vec2(-1.0 * normal.y, normal.x);

  for (int j = 0; j < 4; j++)
  {
    int i = j / 2;

    vertexColorGSOutput = vertexColorVSOutput[i];

    gl_Position = vec4(gl_in[i].gl_Position.xy +
        (lineWidthNVC * normal) * ((j + 1) % 2 - 0.5) * gl_in[i].gl_Position.w,
      gl_in[i].gl_Position.z, gl_in[i].gl_Position.w);

    EmitVertex();
  }
  EndPrimitive();
}
