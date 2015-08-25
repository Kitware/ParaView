#include "vtkPointGaussianRepresentation.h"

#include "vtkActor.h"
#include "vtkAlgorithmOutput.h"
#include "vtkCompositeDataSet.h"
#include "vtkCompositeDataToUnstructuredGridFilter.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMaskPoints.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointGaussianMapper.h"
#include "vtkProperty.h"
#include "vtkPVRenderView.h"
#include "vtkRenderer.h"
#include "vtkUnstructuredGrid.h"
#include "vtkDataSet.h"
#include "vtkPiecewiseFunction.h"

vtkStandardNewMacro(vtkPointGaussianRepresentation)

//----------------------------------------------------------------------------
vtkPointGaussianRepresentation::vtkPointGaussianRepresentation()
{
  this->Mapper = vtkSmartPointer< vtkPointGaussianMapper >::New();
  this->Actor = vtkSmartPointer< vtkActor >::New();
  this->Actor->SetMapper(this->Mapper);
  this->ScaleByArray = false;
  this->LastScaleArray = NULL;
  this->OpacityByArray = false;
  this->LastOpacityArray = NULL;
}

//----------------------------------------------------------------------------
vtkPointGaussianRepresentation::~vtkPointGaussianRepresentation()
{
  this->SetLastScaleArray(NULL);
  this->SetLastOpacityArray(NULL);
}

//----------------------------------------------------------------------------
void vtkPointGaussianRepresentation::PrintSelf(ostream &os, vtkIndent indent)
{
  os << "vtkPointGaussianRepresentation: {" << std::endl;
  this->Superclass::PrintSelf(os,indent);
  os << "}" << std::endl;
}

//----------------------------------------------------------------------------
bool vtkPointGaussianRepresentation::AddToView(vtkView* view)
{
  vtkPVRenderView* rview = vtkPVRenderView::SafeDownCast(view);
  if (rview)
    {
    rview->GetRenderer()->AddActor(this->Actor);
    return true;
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
    return true;
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
    vtkInformation *request, vtkInformationVector **inputVector,
    vtkInformationVector *outputVector)
{
    vtkSmartPointer<vtkDataSet> input = vtkDataSet::GetData(inputVector[0]);
    vtkPolyData* inputPolyData = vtkPolyData::SafeDownCast(input);
    vtkCompositeDataSet* compositeInput = vtkCompositeDataSet::GetData(inputVector[0],0);
    this->ProcessedData = NULL;
    if(inputPolyData)
        {
        this->ProcessedData = inputPolyData;
        }
    else if (compositeInput)
        {
        vtkNew< vtkCompositeDataToUnstructuredGridFilter > merge;
        merge->SetInputData(compositeInput);
        merge->Update();
        input = merge->GetOutput();
        }

    // The mapper underneath expect only PolyData
    // Apply conversion - We do not need vertex list as we
    // use all the points in that use case
    if(this->ProcessedData == NULL && input != NULL)
        {
        vtkNew< vtkMaskPoints > unstructuredToPolyData;
        unstructuredToPolyData->SetInputData(input);
        unstructuredToPolyData->SetMaximumNumberOfPoints(input->GetNumberOfPoints());
        unstructuredToPolyData->GenerateVerticesOff();
        unstructuredToPolyData->SetOnRatio(1);
        unstructuredToPolyData->Update();
        this->ProcessedData = unstructuredToPolyData->GetOutput();
        }

    return this->Superclass::RequestData(request,inputVector,outputVector);
}

//----------------------------------------------------------------------------
int vtkPointGaussianRepresentation::ProcessViewRequest(
    vtkInformationRequestKey *request_type, vtkInformation *inInfo,
    vtkInformation *outInfo)
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
    double bounds[6] = { 0, 0, 0, 0, 0, 0};
    // Standard representation stuff, first.
    // 1. Provide the data being rendered.
    if (this->ProcessedData)
      {
      vtkPVRenderView::SetPiece(inInfo, this, this->ProcessedData);
      this->ProcessedData->GetBounds(bounds);
      }
    else
      {
      // the mapper doesn't handle NULL input data, so don't pass NULL
      vtkNew< vtkPolyData > tmpData;
      vtkPVRenderView::SetPiece(inInfo, this, tmpData.GetPointer());
      }
    // 2. Provide the bounds.
    vtkPVRenderView::SetGeometryBounds(inInfo, bounds);
    outInfo->Set(vtkPVRenderView::NEED_ORDERED_COMPOSITING(), 1);
    }
  else if (request_type == vtkPVView::REQUEST_RENDER())
    {
      vtkAlgorithmOutput* producerPort = vtkPVRenderView::GetPieceProducer(inInfo, this);

      this->Mapper->SetInputConnection(producerPort);
    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkPointGaussianRepresentation::SetInputArrayToProcess(
  int idx, int port, int connection, int fieldAssociation, const char *name)
{
  this->Superclass::SetInputArrayToProcess(
    idx, port, connection, fieldAssociation, name);

  if (name && name[0])
    {
    this->Mapper->SetScalarVisibility(1);
    this->Mapper->SelectColorArray(name);
    this->Mapper->SetUseLookupTableScalarRange(1);
    }
  else
    {
    this->Mapper->SetScalarVisibility(0);
    this->Mapper->SelectColorArray(static_cast<const char*>(NULL));
    }

  switch (fieldAssociation)
    {
  case vtkDataObject::FIELD_ASSOCIATION_CELLS:
    vtkWarningMacro(<<"Using cell data in PointGaussian representation");
    this->Mapper->SetScalarMode(VTK_SCALAR_MODE_USE_CELL_FIELD_DATA);
    break;

  case vtkDataObject::FIELD_ASSOCIATION_POINTS:
  default:
    this->Mapper->SetScalarMode(VTK_SCALAR_MODE_USE_POINT_FIELD_DATA);
    break;
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
  this->Mapper->SetSplatShaderCode(shaderString);
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
    this->Mapper->SetScaleArray(this->ScaleByArray ? this->LastScaleArray : NULL);
    }
}

//----------------------------------------------------------------------------
void vtkPointGaussianRepresentation::SetScaleTransferFunction(vtkPiecewiseFunction* pwf)
{
  this->Mapper->SetScaleFunction(pwf);
}

//----------------------------------------------------------------------------
void vtkPointGaussianRepresentation::SelectScaleArray(int, int, int, int, const char *name)
{
  this->SetLastScaleArray(name);
  this->Mapper->SetScaleArray(this->ScaleByArray ? name : NULL);
}

//----------------------------------------------------------------------------
void vtkPointGaussianRepresentation::SetOpacityByArray(bool newVal)
{
  if (this->OpacityByArray != newVal)
    {
    this->OpacityByArray = newVal;
    this->Modified();
    this->Mapper->SetOpacityArray(this->OpacityByArray ? this->LastOpacityArray : NULL);
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
  this->Mapper->SetOpacityArray(this->OpacityByArray ? name : NULL);
}

//----------------------------------------------------------------------------
#define vtkForwardActorCallMacro(actorMethod,arg,arg_type) \
  void vtkPointGaussianRepresentation::actorMethod(arg_type arg) \
  { this->Actor->actorMethod(arg); }
#define vtkForwardActorCallMacro3Args(actorMethod,arg_type) \
  void vtkPointGaussianRepresentation::actorMethod(arg_type a, arg_type b, arg_type c) \
  { this->Actor->actorMethod(a, b, c); }
#define vtkForwardPropertyCallMacro(propertyMethod,arg,arg_type) \
  void vtkPointGaussianRepresentation::propertyMethod(arg_type arg) \
  { this->Actor->GetProperty()->propertyMethod(arg); }
#define vtkForwardPropertyCallMacro3Args(propertyMethod,arg_type) \
  void vtkPointGaussianRepresentation::propertyMethod(arg_type a, arg_type b, arg_type c) \
  { this->Actor->GetProperty()->propertyMethod(a,b,c); }

vtkForwardActorCallMacro3Args(SetOrientation,      double);
vtkForwardActorCallMacro3Args(SetOrigin,           double);
vtkForwardActorCallMacro3Args(SetPosition,         double);
vtkForwardActorCallMacro3Args(SetScale,            double);

vtkForwardActorCallMacro(SetPickable,              value, int);

vtkForwardPropertyCallMacro3Args(SetColor,         double);
vtkForwardPropertyCallMacro3Args(SetAmbientColor,  double);
vtkForwardPropertyCallMacro3Args(SetDiffuseColor,  double);
vtkForwardPropertyCallMacro3Args(SetSpecularColor, double);
vtkForwardPropertyCallMacro3Args(SetEdgeColor,     double);

vtkForwardPropertyCallMacro(SetOpacity,            value, double);
vtkForwardPropertyCallMacro(SetInterpolation,      value, int);
vtkForwardPropertyCallMacro(SetLineWidth,          value, double);
vtkForwardPropertyCallMacro(SetPointSize,          value, double);
vtkForwardPropertyCallMacro(SetSpecularPower,      value, double);
