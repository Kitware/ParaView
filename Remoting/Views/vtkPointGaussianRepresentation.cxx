#include "vtkPointGaussianRepresentation.h"

#include "vtkActor.h"
#include "vtkAlgorithmOutput.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataSet.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkMaskPoints.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVRenderView.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPointData.h"
#include "vtkPointGaussianMapper.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"

// FIXME: remove once paraview/paraview#20385 is fixed.
#define USE_VERTEX_CELLS 1

vtkStandardNewMacro(vtkPointGaussianRepresentation);
//----------------------------------------------------------------------------
vtkPointGaussianRepresentation::vtkPointGaussianRepresentation()
{
  this->Mapper = vtkSmartPointer<vtkPointGaussianMapper>::New();
  this->Actor = vtkSmartPointer<vtkActor>::New();
  this->Actor->SetMapper(this->Mapper);
  this->ScaleByArray = false;
  this->LastScaleArray = nullptr;
  this->LastScaleArrayComponent = 0;
  this->OpacityByArray = false;
  this->LastOpacityArray = nullptr;
  this->UseScaleFunction = true;
  this->SelectedPreset = vtkPointGaussianRepresentation::GAUSSIAN_BLUR;
  InitializeShaderPresets();
}

//----------------------------------------------------------------------------
vtkPointGaussianRepresentation::~vtkPointGaussianRepresentation()
{
  this->SetLastScaleArray(nullptr);
  this->SetLastOpacityArray(nullptr);
}

//----------------------------------------------------------------------------
void vtkPointGaussianRepresentation::InitializeShaderPresets()
{
  this->PresetShaderStrings.resize(vtkPointGaussianRepresentation::NUMBER_OF_PRESETS);
  this->PresetShaderScales.resize(vtkPointGaussianRepresentation::NUMBER_OF_PRESETS);

  this->PresetShaderStrings[vtkPointGaussianRepresentation::GAUSSIAN_BLUR] = "";
  this->PresetShaderScales[vtkPointGaussianRepresentation::GAUSSIAN_BLUR] = 3.0;

  this->PresetShaderStrings[vtkPointGaussianRepresentation::SPHERE] =
    "//VTK::Color::Impl\n"
    "float dist = dot(offsetVCVSOutput.xy,offsetVCVSOutput.xy);\n"
    "if (dist > 1.0) {\n"
    "  discard;\n"
    "} else {\n"
    "  float scale = (1.0 - dist);\n"
    "  ambientColor *= scale;\n"
    "  diffuseColor *= scale;\n"
    "}\n";
  this->PresetShaderScales[vtkPointGaussianRepresentation::SPHERE] = 1.0;

  this->PresetShaderStrings[vtkPointGaussianRepresentation::BLACK_EDGED_CIRCLE] =
    "//VTK::Color::Impl\n"
    "float dist = dot(offsetVCVSOutput.xy,offsetVCVSOutput.xy);\n"
    "if (dist > 1.0) {\n"
    "  discard;\n"
    "} else if (dist > 0.8) {\n"
    "  ambientColor = vec3(0.0, 0.0, 0.0);\n"
    "  diffuseColor = vec3(0.0, 0.0, 0.0);\n"
    "}\n";
  this->PresetShaderScales[vtkPointGaussianRepresentation::BLACK_EDGED_CIRCLE] = 1.0;

  this->PresetShaderStrings[vtkPointGaussianRepresentation::PLAIN_CIRCLE] =
    "//VTK::Color::Impl\n"
    "float dist = dot(offsetVCVSOutput.xy,offsetVCVSOutput.xy);\n"
    "if (dist > 1.0) {\n"
    "  discard;\n"
    "};\n";
  this->PresetShaderScales[vtkPointGaussianRepresentation::PLAIN_CIRCLE] = 1.0;

  this->PresetShaderStrings[vtkPointGaussianRepresentation::TRIANGLE] = "//VTK::Color::Impl\n";
  this->PresetShaderScales[vtkPointGaussianRepresentation::TRIANGLE] = 1.0;

  this->PresetShaderStrings[vtkPointGaussianRepresentation::SQUARE_OUTLINE] =
    "//VTK::Color::Impl\n"
    "if (abs(offsetVCVSOutput.x) > 2.2 || abs(offsetVCVSOutput.y) > 2.2) {\n"
    "  discard;\n"
    "}\n"
    "if (abs(offsetVCVSOutput.x) < 1.5 && abs(offsetVCVSOutput.y) < 1.5) {\n"
    "  discard;\n"
    "}\n";
  this->PresetShaderScales[vtkPointGaussianRepresentation::SQUARE_OUTLINE] = 3.0;

  this->PresetShaderStrings[vtkPointGaussianRepresentation::CUSTOM] =
    "//VTK::Color::Impl\n"
    "float dist2 = dot(offsetVCVSOutput.xy,offsetVCVSOutput.xy);\n"
    "if (dist2 > 9.0) {\n"
    "  discard;\n"
    "}\n"
    "float gaussian = exp(-0.5*dist2);\n"
    "opacity = opacity*gaussian;\n";

  this->PresetShaderStrings[vtkPointGaussianRepresentation::CUSTOM] = "//VTK::Color::Impl\n";
  this->PresetShaderScales[vtkPointGaussianRepresentation::CUSTOM] = 1.0;
}

//----------------------------------------------------------------------------
void vtkPointGaussianRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  os << "vtkPointGaussianRepresentation: {" << std::endl;
  this->Superclass::PrintSelf(os, indent);
  os << "}" << std::endl;
}

//----------------------------------------------------------------------------
bool vtkPointGaussianRepresentation::AddToView(vtkView* view)
{
  vtkPVRenderView* rview = vtkPVRenderView::SafeDownCast(view);
  if (rview)
  {
    rview->GetRenderer()->AddActor(this->Actor);
    rview->RegisterPropForHardwareSelection(this, this->Actor);
    return this->Superclass::AddToView(view);
  }
  return false;
}

//----------------------------------------------------------------------------
bool vtkPointGaussianRepresentation::RemoveFromView(vtkView* view)
{
  vtkPVRenderView* rview = vtkPVRenderView::SafeDownCast(view);
  if (rview)
  {
    rview->GetRenderer()->RemoveActor(this->Actor);
    return this->Superclass::RemoveFromView(view);
  }
  return false;
}

//----------------------------------------------------------------------------
void vtkPointGaussianRepresentation::SetVisibility(bool val)
{
  this->Actor->SetVisibility(val);
  this->Superclass::SetVisibility(val);
}

//----------------------------------------------------------------------------
void vtkPointGaussianRepresentation::SetEmissive(bool val)
{
  this->Mapper->SetEmissive(val);
}

//----------------------------------------------------------------------------
void vtkPointGaussianRepresentation::SetMapScalars(int val)
{
  if (val != 0 && val != 1)
  {
    vtkWarningMacro(<< "Invalid parameter for vtkPointGaussianRepresentation::SetMapScalars: "
                    << val);
    val = 0;
  }
  int mapToColorMode[] = { VTK_COLOR_MODE_DIRECT_SCALARS, VTK_COLOR_MODE_MAP_SCALARS };
  this->Mapper->SetColorMode(mapToColorMode[val]);
}

//----------------------------------------------------------------------------
int vtkPointGaussianRepresentation::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");

  // Saying INPUT_IS_OPTIONAL() is essential, since representations don't have
  // any inputs on client-side (in client-server, client-render-server mode) and
  // render-server-side (in client-render-server mode).
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);

  return 1;
}

//----------------------------------------------------------------------------
int vtkPointGaussianRepresentation::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (auto input = vtkDataObject::GetData(inputVector[0], 0))
  {
    vtkNew<vtkMaskPoints> unstructuredToPolyData;
    unstructuredToPolyData->SetInputData(input);
    unstructuredToPolyData->SetMaximumNumberOfPoints(VTK_ID_MAX);
#if USE_VERTEX_CELLS
    unstructuredToPolyData->GenerateVerticesOn();
    unstructuredToPolyData->SingleVertexPerCellOn();
#else
    unstructuredToPolyData->GenerateVerticesOff();
#endif
    unstructuredToPolyData->SetOnRatio(1);
    unstructuredToPolyData->Update();
    this->ProcessedData = unstructuredToPolyData->GetOutputDataObject(0);
    if (vtkCompositeDataSet::SafeDownCast(this->ProcessedData) == nullptr)
    {
      vtkNew<vtkMultiBlockDataSet> ds;
      ds->SetBlock(0, this->ProcessedData);
      this->ProcessedData = ds;
    }
  }

  // Create a vtkMultiBlockDataSet on the client to match what is delivered by the server
  if (this->ProcessedData == nullptr)
  {
    this->ProcessedData = vtkSmartPointer<vtkMultiBlockDataSet>::New();
  }

  return this->Superclass::RequestData(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkPointGaussianRepresentation::ProcessViewRequest(
  vtkInformationRequestKey* request_type, vtkInformation* inInfo, vtkInformation* outInfo)
{
  // always forward to superclass first. Superclass returns 0 if the
  // representation is not visible (among other things). In which case there's
  // nothing to do.
  if (!this->Superclass::ProcessViewRequest(request_type, inInfo, outInfo))
  {
    return 0;
  }

  if (request_type == vtkPVView::REQUEST_UPDATE())
  {
    double bounds[6] = { 0, 0, 0, 0, 0, 0 };

    // Standard representation stuff, first.
    // 1. Provide the data being rendered.
    if (this->ProcessedData)
    {
      vtkCompositeDataSet* cd = vtkCompositeDataSet::SafeDownCast(this->ProcessedData);
      if (cd)
      {
        cd->GetBounds(bounds);
      }
      else
      {
        vtkMath::UninitializeBounds(bounds);
      }

      vtkPVRenderView::SetPiece(inInfo, this, this->ProcessedData);
    }

    // 2. Provide the bounds.
    vtkNew<vtkMatrix4x4> matrix;
    this->Actor->GetMatrix(matrix.GetPointer());
    vtkPVRenderView::SetGeometryBounds(inInfo, this, bounds, matrix.GetPointer());
    outInfo->Set(vtkPVRenderView::NEED_ORDERED_COMPOSITING(), 1);
    vtkPVRenderView::SetOrderedCompositingConfiguration(inInfo, this,
      vtkPVRenderView::DATA_IS_REDISTRIBUTABLE | vtkPVRenderView::USE_DATA_FOR_LOAD_BALANCING);
  }
  else if (request_type == vtkPVView::REQUEST_RENDER())
  {
    auto data = vtkPVView::GetDeliveredPiece(inInfo, this);
    this->Mapper->SetInputDataObject(data);
    this->UpdateColoringParameters();
  }
  return 1;
}

//----------------------------------------------------------------------------
void vtkPointGaussianRepresentation::UpdateColoringParameters()
{
  vtkInformation* info = this->GetInputArrayInformation(0);
  if (info && info->Has(vtkDataObject::FIELD_ASSOCIATION()) &&
    info->Has(vtkDataObject::FIELD_NAME()))
  {
    const char* colorArrayName = info->Get(vtkDataObject::FIELD_NAME());
    int fieldAssociation = info->Get(vtkDataObject::FIELD_ASSOCIATION());
    if (colorArrayName && colorArrayName[0])
    {
      this->Mapper->SetScalarVisibility(1);
      this->Mapper->SelectColorArray(colorArrayName);
      this->Mapper->SetUseLookupTableScalarRange(1);
    }
    else
    {
      this->Mapper->SetScalarVisibility(0);
      this->Mapper->SelectColorArray(static_cast<const char*>(nullptr));
    }

    switch (fieldAssociation)
    {
      case vtkDataObject::FIELD_ASSOCIATION_CELLS:
        this->Mapper->SetScalarVisibility(0);
        this->Mapper->SelectColorArray(static_cast<const char*>(nullptr));
        break;

      case vtkDataObject::FIELD_ASSOCIATION_POINTS:
      default:
        this->Mapper->SetScalarMode(VTK_SCALAR_MODE_USE_POINT_FIELD_DATA);
        break;
    }
  }
}

//----------------------------------------------------------------------------
void vtkPointGaussianRepresentation::SetLookupTable(vtkScalarsToColors* lut)
{
  this->Mapper->SetLookupTable(lut);
}

//----------------------------------------------------------------------------
void vtkPointGaussianRepresentation::SetCustomShader(const char* shaderString)
{
  this->PresetShaderStrings[vtkPointGaussianRepresentation::CUSTOM] = shaderString;
  if (this->SelectedPreset == vtkPointGaussianRepresentation::CUSTOM)
  {
    this->Mapper->SetSplatShaderCode(this->PresetShaderStrings[this->SelectedPreset].c_str());
  }
}

//----------------------------------------------------------------------------
void vtkPointGaussianRepresentation::SetCustomTriangleScale(double scale)
{
  this->PresetShaderScales[vtkPointGaussianRepresentation::CUSTOM] = scale;
  if (this->SelectedPreset == vtkPointGaussianRepresentation::CUSTOM)
  {
    this->Mapper->SetTriangleScale(this->PresetShaderScales[this->SelectedPreset]);
  }
}

//----------------------------------------------------------------------------
void vtkPointGaussianRepresentation::SelectShaderPreset(int preset)
{
  if (preset != this->SelectedPreset)
  {
    this->SelectedPreset = preset;
    this->Mapper->SetSplatShaderCode(this->PresetShaderStrings[preset].c_str());
    this->Mapper->SetTriangleScale(this->PresetShaderScales[preset]);
  }
}

//----------------------------------------------------------------------------
void vtkPointGaussianRepresentation::SetSplatSize(double radius)
{
  this->Mapper->SetScaleFactor(radius);
}

//----------------------------------------------------------------------------
void vtkPointGaussianRepresentation::SetScaleByArray(bool newVal)
{
  if (this->ScaleByArray != newVal)
  {
    this->ScaleByArray = newVal;
    this->Modified();
    this->Mapper->SetScaleArray(this->ScaleByArray ? this->LastScaleArray : nullptr);
    this->Mapper->SetScaleArrayComponent(this->ScaleByArray ? this->LastScaleArrayComponent : 0);
  }
}

//----------------------------------------------------------------------------
void vtkPointGaussianRepresentation::SetUseScaleFunction(bool enable)
{
  if (this->UseScaleFunction != enable)
  {
    this->UseScaleFunction = enable;
    this->Modified();
    this->UpdateMapperScaleFunction();
  }
}

//----------------------------------------------------------------------------
void vtkPointGaussianRepresentation::SetScaleTransferFunction(vtkPiecewiseFunction* pwf)
{
  if (this->ScaleFunction.Get() != pwf)
  {
    this->ScaleFunction = pwf;
    this->Modified();
    this->UpdateMapperScaleFunction();
  }
}

//----------------------------------------------------------------------------
void vtkPointGaussianRepresentation::UpdateMapperScaleFunction()
{
  this->Mapper->SetScaleFunction(this->UseScaleFunction ? this->ScaleFunction : nullptr);
}

//----------------------------------------------------------------------------
void vtkPointGaussianRepresentation::SelectScaleArray(int, int, int, int, const char* name)
{
  this->SetLastScaleArray(name);
  this->Mapper->SetScaleArray(this->ScaleByArray ? name : nullptr);
}

//----------------------------------------------------------------------------
void vtkPointGaussianRepresentation::SelectScaleArrayComponent(int component)
{
  this->LastScaleArrayComponent = component;
  this->Mapper->SetScaleArrayComponent(this->ScaleByArray ? component : 0);
}

//----------------------------------------------------------------------------
void vtkPointGaussianRepresentation::SetOpacityByArray(bool newVal)
{
  if (this->OpacityByArray != newVal)
  {
    this->OpacityByArray = newVal;
    this->Modified();
    this->Mapper->SetOpacityArray(this->OpacityByArray ? this->LastOpacityArray : nullptr);
    this->Mapper->SetOpacityArrayComponent(
      this->OpacityByArray ? this->LastOpacityArrayComponent : 0);
  }
}

//----------------------------------------------------------------------------
void vtkPointGaussianRepresentation::SetOpacityTransferFunction(vtkPiecewiseFunction* pwf)
{
  this->Mapper->SetScalarOpacityFunction(pwf);
}

//----------------------------------------------------------------------------
void vtkPointGaussianRepresentation::SelectOpacityArray(int, int, int, int, const char* name)
{
  this->SetLastOpacityArray(name);
  this->Mapper->SetOpacityArray(this->OpacityByArray ? name : nullptr);
}

//----------------------------------------------------------------------------
void vtkPointGaussianRepresentation::SelectOpacityArrayComponent(int component)
{
  this->LastOpacityArrayComponent = component;
  this->Mapper->SetOpacityArrayComponent(this->OpacityByArray ? component : 0);
}

//----------------------------------------------------------------------------
#define vtkForwardActorCallMacro(actorMethod, arg, arg_type)                                       \
  void vtkPointGaussianRepresentation::actorMethod(arg_type arg) { this->Actor->actorMethod(arg); }
#define vtkForwardActorCallMacro3Args(actorMethod, arg_type)                                       \
  void vtkPointGaussianRepresentation::actorMethod(arg_type a, arg_type b, arg_type c)             \
  {                                                                                                \
    this->Actor->actorMethod(a, b, c);                                                             \
  }
#define vtkForwardPropertyCallMacro(propertyMethod, arg, arg_type)                                 \
  void vtkPointGaussianRepresentation::propertyMethod(arg_type arg)                                \
  {                                                                                                \
    this->Actor->GetProperty()->propertyMethod(arg);                                               \
  }
#define vtkForwardPropertyCallMacro3Args(propertyMethod, arg_type)                                 \
  void vtkPointGaussianRepresentation::propertyMethod(arg_type a, arg_type b, arg_type c)          \
  {                                                                                                \
    this->Actor->GetProperty()->propertyMethod(a, b, c);                                           \
  }

vtkForwardActorCallMacro3Args(SetOrientation, double);
vtkForwardActorCallMacro3Args(SetOrigin, double);
vtkForwardActorCallMacro3Args(SetPosition, double);
vtkForwardActorCallMacro3Args(SetScale, double);

vtkForwardActorCallMacro(SetPickable, value, int);

vtkForwardPropertyCallMacro3Args(SetAmbientColor, double);
vtkForwardPropertyCallMacro3Args(SetDiffuseColor, double);
vtkForwardPropertyCallMacro3Args(SetSpecularColor, double);
vtkForwardPropertyCallMacro3Args(SetEdgeColor, double);

vtkForwardPropertyCallMacro(SetOpacity, value, double);
vtkForwardPropertyCallMacro(SetInterpolation, value, int);
vtkForwardPropertyCallMacro(SetLineWidth, value, double);
vtkForwardPropertyCallMacro(SetPointSize, value, double);
vtkForwardPropertyCallMacro(SetSpecularPower, value, double);
