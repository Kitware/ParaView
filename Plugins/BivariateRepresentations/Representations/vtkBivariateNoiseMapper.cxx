/*=========================================================================

  Program:   ParaView
  Module:    vtkBivariateNoiseMapper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkBivariateNoiseMapper.h"
#include "vtkCompositeMapperHelper2.h"
#include "vtkFloatArray.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLUniforms.h"
#include "vtkOpenGLVertexBufferObjectGroup.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRenderer.h"
#include "vtkShader.h"
#include "vtkShaderProgram.h"
#include "vtkShaderProperty.h"

#include <chrono>

//----------------------------------------------------------------------------
struct vtkBivariateNoiseMapper::vtkInternals
{
  double Frequency = 30.0;
  double Amplitude = 0.5;
  double Speed = 1.0;
  int NbOfOctaves = 3;
  long StartTime = 0;
  bool Initialized = false;
};

//----------------------------------------------------------------------------
/**
 * The vtkBivariateNoiseMapperHelper inheritate indirectly from vtkOpenGLPolydataMapper
 * and contains most of the rendering code specific to the vtkBivariateNoiseMapper.
 */
class vtkBivariateNoiseMapperHelper : public vtkCompositeMapperHelper2
{
public:
  static vtkBivariateNoiseMapperHelper* New();
  vtkTypeMacro(vtkBivariateNoiseMapperHelper, vtkCompositeMapperHelper2);

protected:
  vtkBivariateNoiseMapperHelper() = default;
  ~vtkBivariateNoiseMapperHelper() override = default;

  /**
   * Contain most of the shader replacements specific to this mapper.
   * Define the noise function and use it to generate the output of the fragment shader.
   */
  void ReplaceShaderColor(
    std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* act) override;

  /**
   * Pass and interpolate vertex positions (in model coordinates)
   * from the vertex shader to the fragment shader.
   */
  void ReplaceShaderPositionVC(
    std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* actor) override;

  /**
   * Define the custom uniforms in the fragment shader (frequency, amplitude, time).
   */
  void ReplaceShaderCustomUniforms(
    std::map<vtkShader::Type, vtkShader*> shaders, vtkActor* actor) override;

  /**
   * Set the custom uniforms values in the fragment shader.
   */
  void SetCustomUniforms(vtkOpenGLHelper& cellBO, vtkActor* actor) override;

  /**
   * Pass the noise array on the OpenGL side as a VBO.
   */
  void AppendOneBufferObject(vtkRenderer* ren, vtkActor* act, vtkCompositeMapperHelperData* hdata,
    vtkIdType& flat_index, std::vector<unsigned char>& colors, std::vector<float>& norms) override;

private:
  vtkBivariateNoiseMapperHelper(const vtkBivariateNoiseMapperHelper&) = delete;
  void operator=(const vtkBivariateNoiseMapperHelper&) = delete;
};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkBivariateNoiseMapperHelper);

//----------------------------------------------------------------------------
void vtkBivariateNoiseMapperHelper::ReplaceShaderColor(
  std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* act)
{
  // InterpolateScalarsBeforeMapping: noise effect is applied during color lookup in the FS.
  // ColorCoordinates: we need the first array values.
  if (this->InterpolateScalarsBeforeMapping && this->ColorCoordinates && !this->DrawingVertices)
  {
    // The bivariate data correspond to the second array to show
    // in the form of Perlin noise intensity.
    std::string colorDecVS =
      R"(
in float bivariateData;
out float vertexBivariateDataVSOut;
)";

    // The bivariate data should be interpolated between VS and FS.
    // We interpolate the scalars before mapping the colors in the FS.
    std::string colorImplVS =
      R"(
  vertexBivariateDataVSOut = bivariateData;
  )";

    // Here we define our functions (Perlin 4D noise + utilities).
    std::string colorDecFS =
      R"(
vec4 random4D(in vec4 inVec)
{
  // Dot products of the coordinates with arbitrary vectors
  float dot1 = dot(inVec, vec4(152.235, 478.267, -574.241, 342.365));
  float dot2 = dot(inVec, vec4(328.438, 575.981, 124.254, -132.43));
  float dot3 = dot(inVec, vec4(-28.438, -175.981, 287.399, 45.201));
  float dot4 = dot(inVec, vec4(522.378, -2.358, 355.247, -123.321));

  // Multiply sin with big number, keep the fractionnal part: pseudo-random vector
  // *2 - 1 : ensure negative coordinates are possible
  return fract(sin(vec4(dot1, dot2, dot3, dot4)) * 458724.) * 2. - vec4(1., 1., 1., 1.);
}

// Use quintic Hermite interpolation for smoother results than the classic
// smoothstep function (that uses cubic Hermite interpolation)
float quinticSmooth(in float t)
{
  // No clamp needed like smoothstep as we are garanteed
  // to have t between 0. and 1. here
  return t * t * t *(t * (t * 6. -15.) + 10.);
}

float noise(in vec4 inVec)
{
  // Interger and fractional parts
  vec4 i_inVec = floor(inVec); // "4D tile" where inVec belongs
  vec4 f_inVec = fract(inVec); // Where in the tile (between 0. and 1.)

  // Random values at corners of the 4D tile where inVec belongs
  float val0000 = dot(random4D(i_inVec + vec4(0., 0., 0., 0.)), f_inVec - vec4(0., 0., 0., 0.));
  float val0001 = dot(random4D(i_inVec + vec4(0., 0., 0., 1.)), f_inVec - vec4(0., 0., 0., 1.));
  float val0010 = dot(random4D(i_inVec + vec4(0., 0., 1., 0.)), f_inVec - vec4(0., 0., 1., 0.));
  float val0011 = dot(random4D(i_inVec + vec4(0., 0., 1., 1.)), f_inVec - vec4(0., 0., 1., 1.));
  float val0100 = dot(random4D(i_inVec + vec4(0., 1., 0., 0.)), f_inVec - vec4(0., 1., 0., 0.));
  float val0101 = dot(random4D(i_inVec + vec4(0., 1., 0., 1.)), f_inVec - vec4(0., 1., 0., 1.));
  float val0110 = dot(random4D(i_inVec + vec4(0., 1., 1., 0.)), f_inVec - vec4(0., 1., 1., 0.));
  float val0111 = dot(random4D(i_inVec + vec4(0., 1., 1., 1.)), f_inVec - vec4(0., 1., 1., 1.));
  float val1000 = dot(random4D(i_inVec + vec4(1., 0., 0., 0.)), f_inVec - vec4(1., 0., 0., 0.));
  float val1001 = dot(random4D(i_inVec + vec4(1., 0., 0., 1.)), f_inVec - vec4(1., 0., 0., 1.));
  float val1010 = dot(random4D(i_inVec + vec4(1., 0., 1., 0.)), f_inVec - vec4(1., 0., 1., 0.));
  float val1011 = dot(random4D(i_inVec + vec4(1., 0., 1., 1.)), f_inVec - vec4(1., 0., 1., 1.));
  float val1100 = dot(random4D(i_inVec + vec4(1., 1., 0., 0.)), f_inVec - vec4(1., 1., 0., 0.));
  float val1101 = dot(random4D(i_inVec + vec4(1., 1., 0., 1.)), f_inVec - vec4(1., 1., 0., 1.));
  float val1110 = dot(random4D(i_inVec + vec4(1., 1., 1., 0.)), f_inVec - vec4(1., 1., 1., 0.));
  float val1111 = dot(random4D(i_inVec + vec4(1., 1., 1., 1.)), f_inVec - vec4(1., 1., 1., 1.));

  // Smooth interpolation along each axis of the 4D tile
  float x_smooth = quinticSmooth(f_inVec.x);
  float y_smooth = quinticSmooth(f_inVec.y);
  float z_smooth = quinticSmooth(f_inVec.z);
  float w_smooth = quinticSmooth(f_inVec.w);

  // Quadrilinear interpolation
  float x_inter1 = mix(val0000, val1000, x_smooth);
  float x_inter2 = mix(val0100, val1100, x_smooth);
  float x_inter3 = mix(val0010, val1010, x_smooth);
  float x_inter4 = mix(val0110, val1110, x_smooth);
  float x_inter5 = mix(val0001, val1001, x_smooth);
  float x_inter6 = mix(val0101, val1101, x_smooth);
  float x_inter7 = mix(val0011, val1011, x_smooth);
  float x_inter8 = mix(val0111, val1111, x_smooth);

  float y_inter1 = mix(x_inter1, x_inter2, y_smooth);
  float y_inter2 = mix(x_inter3, x_inter4, y_smooth);
  float y_inter3 = mix(x_inter5, x_inter6, y_smooth);
  float y_inter4 = mix(x_inter7, x_inter8, y_smooth);

  float z_inter1 = mix(y_inter1, y_inter2, z_smooth);
  float z_inter2 = mix(y_inter3, y_inter4, z_smooth);

  return mix(z_inter1, z_inter2, w_smooth);
}

// Fractal noise
// Add "layers" of noise by iterating over octaves.
float fNoise (in vec4 inVec, in int nbOfOctaves)
{
  float result = 0.;
  float freqMultiplier = 1.;
  float ampMultiplier = 1.;

  // Loop over each octave. At each step,
  // double the frequency and divide by 2 the amplitude.
  for (int i = 0; i < nbOfOctaves; i++)
  {
    result += ampMultiplier * noise(inVec * freqMultiplier);
    freqMultiplier *= 2.;
    ampMultiplier *= 0.5;
  }
  return result;
}

in float vertexBivariateDataVSOut;
)";

    // Here we apply the value of the 4D Perlin noise (depending on the noise array)
    // on the value of 2D texture coordinates (tcoordVCVSOutput.s corresponds to the 1st data
    // array with values clamped between 0.0 and 1.0, tcoordVCVSOutput.t is always equal to 0).
    // This ends up oscillating over the color texture (1D).
    // Bigger is the noise array value, bigger is the amplitude.
    std::string colorImplFS =
      R"(
  // Input vector: 3D coordinates + time.
  vec4 inVec = vec4(vertexMCVSOutput.xyz * frequency, currentTime * speed);

  // Compute and apply the noise value to modify the color texture coordinates.
  float noise = fNoise(inVec, nbOfOctaves);
  vec2 _texCoord = tcoordVCVSOutput.st + vec2(noise, 0.) * amplitude * vertexBivariateDataVSOut;
  )";

    vtkShaderProgram::Substitute(
      shaders[vtkShader::Vertex], "//VTK::Color::Dec", colorDecVS + "\n//VTK::Color::Dec\n");
    vtkShaderProgram::Substitute(
      shaders[vtkShader::Vertex], "//VTK::Color::Impl", colorImplVS + "\n//VTK::Color::Impl\n");
    vtkShaderProgram::Substitute(
      shaders[vtkShader::Fragment], "//VTK::Color::Dec", colorDecFS + "\n//VTK::Color::Dec\n");
    vtkShaderProgram::Substitute(
      shaders[vtkShader::Fragment], "//VTK::Color::Impl", colorImplFS + "\n//VTK::Color::Impl\n");

    this->Superclass::ReplaceShaderColor(shaders, ren, act);

    // Now we need to override the texColor value (after the replacements defined in
    // vtkOpenGLPolyDataMapper) in order to use our custom _texCoord value.
    vtkShaderProgram::Substitute(shaders[vtkShader::Fragment],
      "vec4 texColor = texture(colortexture, tcoordVCVSOutput.st);",
      "vec4 texColor = texture(colortexture, _texCoord);");
  }
  else
  {
    // Just call superclass
    this->Superclass::ReplaceShaderColor(shaders, ren, act);
  }
}

//----------------------------------------------------------------------------
void vtkBivariateNoiseMapperHelper::ReplaceShaderPositionVC(
  std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* actor)
{
  // We need to interpolate the vertex positions (in model coordinate)
  // between the vertex shader and the fragment shader, because the Perlin
  // noise function take it as a parameter.
  vtkShaderProgram::Substitute(shaders[vtkShader::Vertex], "//VTK::PositionVC::Dec",
    R"(
//VTK::PositionVC::Dec
out vec4 vertexMCVSOutput;
)");

  vtkShaderProgram::Substitute(shaders[vtkShader::Vertex], "//VTK::PositionVC::Impl",
    R"(
  //VTK::PositionVC::Impl
  vertexMCVSOutput = vertexMC;
  )");

  // Fragment shader substitutions
  vtkShaderProgram::Substitute(shaders[vtkShader::Fragment], "//VTK::PositionVC::Dec",
    R"(
//VTK::PositionVC::Dec
in vec4 vertexMCVSOutput;
)");

  this->Superclass::ReplaceShaderPositionVC(shaders, ren, actor);
}

//----------------------------------------------------------------------------
void vtkBivariateNoiseMapperHelper::ReplaceShaderCustomUniforms(
  std::map<vtkShader::Type, vtkShader*> shaders, vtkActor* actor)
{
  vtkShaderProgram::Substitute(shaders[vtkShader::Fragment], "//VTK::CustomUniforms::Dec",
    R"(
//VTK::CustomUniforms::Dec
uniform float frequency = 10.;
uniform float amplitude = 0.5;
uniform float speed = 1.;
uniform int nbOfOctaves = 3;
uniform float currentTime = 0.;
)");

  this->Superclass::ReplaceShaderCustomUniforms(shaders, actor);
}

//----------------------------------------------------------------------------
void vtkBivariateNoiseMapperHelper::SetCustomUniforms(vtkOpenGLHelper& cellBO, vtkActor* actor)
{
  this->Superclass::SetCustomUniforms(cellBO, actor);
  vtkBivariateNoiseMapper* parent = vtkBivariateNoiseMapper::SafeDownCast(this->Parent);
  cellBO.Program->SetUniformf("frequency", parent->Internals->Frequency);
  cellBO.Program->SetUniformf("amplitude", parent->Internals->Amplitude);
  cellBO.Program->SetUniformf("speed", parent->Internals->Speed);
  cellBO.Program->SetUniformi("nbOfOctaves", parent->Internals->NbOfOctaves);
  auto time =
    (std::chrono::steady_clock::now().time_since_epoch().count() - parent->Internals->StartTime) *
    0.0000000006; // Value defined empirically for suitable default speed
  cellBO.Program->SetUniformf("currentTime", time);
}

//------------------------------------------------------------------------------
void vtkBivariateNoiseMapperHelper::AppendOneBufferObject(vtkRenderer* ren, vtkActor* act,
  vtkCompositeMapperHelperData* hdata, vtkIdType& voffset, std::vector<unsigned char>& newColors,
  std::vector<float>& newNorms)
{
  vtkPolyData* poly = hdata->Data;
  vtkDataArray* array = this->GetInputArrayToProcess(1, poly); // Noise array
  if (array && array->GetNumberOfComponents() == 1)
  {
    vtkNew<vtkFloatArray> floatArray;
    floatArray->DeepCopy(array);
    this->VBOs->AppendDataArray("bivariateData", floatArray, VTK_FLOAT);
  }

  this->Superclass::AppendOneBufferObject(ren, act, hdata, voffset, newColors, newNorms);
}

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkBivariateNoiseMapper);

//----------------------------------------------------------------------------
vtkBivariateNoiseMapper::vtkBivariateNoiseMapper()
  : Internals(new vtkBivariateNoiseMapper::vtkInternals())
{
}

//----------------------------------------------------------------------------
vtkBivariateNoiseMapper::~vtkBivariateNoiseMapper() = default;

//----------------------------------------------------------------------------
void vtkBivariateNoiseMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Frequency: " << this->Internals->Frequency << endl;
  os << indent << "Amplitude: " << this->Internals->Amplitude << endl;
  os << indent << "Speed: " << this->Internals->Speed << endl;
  os << indent << "Nb of octaves: " << this->Internals->NbOfOctaves << endl;
}

//----------------------------------------------------------------------------
vtkCompositeMapperHelper2* vtkBivariateNoiseMapper::CreateHelper()
{
  return vtkBivariateNoiseMapperHelper::New();
}

//----------------------------------------------------------------------------
void vtkBivariateNoiseMapper::CopyMapperValuesToHelper(vtkCompositeMapperHelper2* helper)
{
  this->Superclass::CopyMapperValuesToHelper(helper);
  helper->SetInputArrayToProcess(1, this->GetInputArrayInformation(1)); // Noise array
}

//----------------------------------------------------------------------------
void vtkBivariateNoiseMapper::SetFrequency(double frequency)
{
  this->Internals->Frequency = frequency;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkBivariateNoiseMapper::SetAmplitude(double amplitude)
{
  this->Internals->Amplitude = amplitude;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkBivariateNoiseMapper::SetSpeed(double speed)
{
  this->Internals->Speed = speed;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkBivariateNoiseMapper::SetNbOfOctaves(int nbOfOctaves)
{
  this->Internals->NbOfOctaves = nbOfOctaves;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkBivariateNoiseMapper::Render(vtkRenderer* ren, vtkActor* act)
{
  if (!this->Internals->Initialized)
  {
    this->Initialize();
  }
  this->Superclass::Render(ren, act);
}

//----------------------------------------------------------------------------
void vtkBivariateNoiseMapper::Initialize()
{
  if (!this->Internals->Initialized)
  {
    this->Internals->StartTime = std::chrono::steady_clock::now().time_since_epoch().count();
    this->Internals->Initialized = true;
    this->Modified();
  }
}
