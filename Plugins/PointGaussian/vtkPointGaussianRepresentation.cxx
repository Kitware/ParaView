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

vtkStandardNewMacro(vtkPointGaussianRepresentation)

vtkPointGaussianRepresentation::vtkPointGaussianRepresentation()
{
  this->Mapper = vtkSmartPointer< vtkPointGaussianMapper >::New();
  this->Actor = vtkSmartPointer< vtkActor >::New();
  this->Actor->SetMapper(this->Mapper);
  this->ScaleByArray = false;
  this->LastScaleArray = NULL;
}

vtkPointGaussianRepresentation::~vtkPointGaussianRepresentation()
{
  this->SetLastScaleArray(NULL);
}

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
  if (inputVector[0]->GetNumberOfInformationObjects() == 1)
    {
    vtkNew< vtkMaskPoints > convertToPoints;
    vtkSmartPointer< vtkDataSet > inData =
        vtkDataSet::GetData(inputVector[0],0);
    vtkCompositeDataSet* inComposite =
        vtkCompositeDataSet::GetData(inputVector[0],0);
    if (inComposite)
      {
      vtkNew< vtkCompositeDataToUnstructuredGridFilter > merge;
      merge->SetInputData(inComposite);
      merge->Update();
      inData = merge->GetOutput();
      }
    convertToPoints->SetInputData(inData);
    convertToPoints->SetMaximumNumberOfPoints(inData->GetNumberOfPoints());
    convertToPoints->GenerateVerticesOn();
    convertToPoints->SingleVertexPerCellOn();
    convertToPoints->SetOnRatio(1);
    convertToPoints->Update();
    this->ProcessedData = convertToPoints->GetOutput();
    }
  else
    {
    this->ProcessedData = NULL;
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
void vtkPointGaussianRepresentation::SetOpacity(double val)
{
  this->Actor->GetProperty()->SetOpacity(val);
}

//----------------------------------------------------------------------------
void vtkPointGaussianRepresentation::SetSplatSize(double radius)
{
  this->Mapper->SetDefaultRadius(radius);
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
void vtkPointGaussianRepresentation::SelectScaleArray(int, int, int, int, const char *name)
{
  this->SetLastScaleArray(name);
  this->Mapper->SetScaleArray(this->ScaleByArray ? name : NULL);
}

//----------------------------------------------------------------------------
void vtkPointGaussianRepresentation::SetColor(double r, double g, double b)
{
  this->Actor->GetProperty()->SetColor(r,g,b);
}

//----------------------------------------------------------------------------
void vtkPointGaussianRepresentation::SetAmbientColor(double r, double g, double b)
{
  this->Actor->GetProperty()->SetAmbientColor(r,g,b);
}

//----------------------------------------------------------------------------
void vtkPointGaussianRepresentation::SetDiffuseColor(double r, double g, double b)
{
  this->Actor->GetProperty()->SetDiffuseColor(r,g,b);
}

//----------------------------------------------------------------------------
void vtkPointGaussianRepresentation::SetSpecularColor(double r, double g, double b)
{
  this->Actor->GetProperty()->SetSpecularColor(r,g,b);
}
