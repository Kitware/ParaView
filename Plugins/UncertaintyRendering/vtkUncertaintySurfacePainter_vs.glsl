#version 120
varying vec4 vColor;
varying vec3 v_texCoord3D;
varying float vUncertainty;
attribute float uncertainty;

// from vtkLightingHelper
vec4 singleColor(gl_MaterialParameters m, vec3 surfacePosEyeCoords, vec3 n);

vec4 colorFrontFace()
{
  vec4 heyeCoords = gl_ModelViewMatrix * gl_Vertex;
  vec3 eyeCoords = heyeCoords.xyz / heyeCoords.w;
  vec3 n = normalize(gl_NormalMatrix * gl_Normal);
  return singleColor(gl_FrontMaterial, eyeCoords, n);
}

void main()
{
  vColor = colorFrontFace();
  v_texCoord3D = gl_Vertex.xyz * 100.0f;
  gl_TexCoord[0] = gl_MultiTexCoord0;
  vUncertainty = uncertainty;
  gl_Position = ftransform();
}
