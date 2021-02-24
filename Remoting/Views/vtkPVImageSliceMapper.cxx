/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVImageSliceMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVImageSliceMapper.h"

#include "vtkCommand.h"
#include "vtkDataArray.h"
#include "vtkExecutive.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkScalarsToColors.h"
#include "vtkScalarsToColors.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include "vtkCellArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkExtractVOI.h"
#include "vtkFloatArray.h"
#include "vtkNew.h"
#include "vtkOpenGLPolyDataMapper.h"
#include "vtkOpenGLTexture.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkTextureObject.h"
#include "vtkTrivialProducer.h"

// no-op  just here to shut up python wrapping
class vtkPainter : public vtkObject
{
};
//-----------------------------------------------------------------------------
void vtkPVImageSliceMapper::SetPainter(vtkPainter*)
{
}

vtkStandardNewMacro(vtkPVImageSliceMapper);
//----------------------------------------------------------------------------
vtkPVImageSliceMapper::vtkPVImageSliceMapper()
{
  this->Piece = 0;
  this->NumberOfPieces = 1;
  this->NumberOfSubPieces = 1;
  this->GhostLevel = 0;

  this->Slice = 0;
  this->SliceMode = XY_PLANE;
  this->UseXYPlane = 0;

  this->Texture = vtkOpenGLTexture::New();
  this->Texture->RepeatOff();
  vtkNew<vtkPolyData> polydata;
  vtkNew<vtkPoints> points;
  points->SetNumberOfPoints(4);
  polydata->SetPoints(points.Get());

  vtkNew<vtkCellArray> tris;
  tris->InsertNextCell(3);
  tris->InsertCellPoint(0);
  tris->InsertCellPoint(1);
  tris->InsertCellPoint(2);
  tris->InsertNextCell(3);
  tris->InsertCellPoint(0);
  tris->InsertCellPoint(2);
  tris->InsertCellPoint(3);
  polydata->SetPolys(tris.Get());

  vtkNew<vtkFloatArray> tcoords;
  tcoords->SetNumberOfComponents(2);
  tcoords->SetNumberOfTuples(4);
  tcoords->SetTuple2(0, 0.0, 0.0);
  tcoords->SetTuple2(1, 1.0, 0.0);
  tcoords->SetTuple2(2, 1.0, 1.0);
  tcoords->SetTuple2(3, 0.0, 1.0);
  polydata->GetPointData()->SetTCoords(tcoords.Get());

  vtkNew<vtkTrivialProducer> prod;
  prod->SetOutput(polydata.Get());
  vtkNew<vtkOpenGLPolyDataMapper> polyDataMapper;
  polyDataMapper->SetInputConnection(prod->GetOutputPort());
  this->PolyDataActor = vtkActor::New();
  this->PolyDataActor->SetMapper(polyDataMapper.Get());
  this->PolyDataActor->SetTexture(this->Texture);
  this->PolyDataActor->GetProperty()->SetAmbient(1.0);
  this->PolyDataActor->GetProperty()->SetDiffuse(0.0);
}

//----------------------------------------------------------------------------
vtkPVImageSliceMapper::~vtkPVImageSliceMapper()
{
  this->Texture->Delete();
  this->Texture = nullptr;
  this->PolyDataActor->Delete();
  this->PolyDataActor = nullptr;
}

//----------------------------------------------------------------------------
static int vtkGetDataDimension(int inextents[6])
{
  int dim[3];
  dim[0] = inextents[1] - inextents[0] + 1;
  dim[1] = inextents[3] - inextents[2] + 1;
  dim[2] = inextents[5] - inextents[4] + 1;
  int dimensionality = 0;
  dimensionality += (dim[0] > 1 ? 1 : 0);
  dimensionality += (dim[1] > 1 ? 1 : 0);
  dimensionality += (dim[2] > 1 ? 1 : 0);
  return dimensionality;
}

static const int XY_PLANE_QPOINTS_INDICES[] = { 0, 2, 4, 1, 2, 4, 1, 3, 4, 0, 3, 4 };
static const int YZ_PLANE_QPOINTS_INDICES[] = { 0, 2, 4, 0, 3, 4, 0, 3, 5, 0, 2, 5 };
static const int XZ_PLANE_QPOINTS_INDICES[] = { 0, 2, 4, 1, 2, 4, 1, 2, 5, 0, 2, 5 };

static const int* XY_PLANE_QPOINTS_INDICES_ORTHO = XY_PLANE_QPOINTS_INDICES;
static const int YZ_PLANE_QPOINTS_INDICES_ORTHO[] = { 2, 4, 0, 3, 4, 0, 3, 5, 0, 2, 5, 0 };
static const int XZ_PLANE_QPOINTS_INDICES_ORTHO[] = { 4, 0, 2, 4, 1, 2, 5, 1, 2, 5, 0, 2 };

int vtkPVImageSliceMapper::SetupScalars(vtkImageData* input)
{
  // Based on the scalar mode, scalar array, scalar id,
  // we need to tell the vtkTexture to use the appropriate scalars.
  int cellFlag = 0;
  vtkDataArray* scalars = vtkAbstractMapper::GetScalars(input, this->ScalarMode,
    *this->ArrayName ? VTK_GET_ARRAY_BY_NAME : VTK_GET_ARRAY_BY_ID, this->ArrayId, this->ArrayName,
    cellFlag);

  if (!scalars)
  {
    vtkWarningMacro("Failed to locate selected scalars. Will use image "
                    "scalars by default.");
    // If not scalar array specified, simply use the point data (the cell
    // data) scalars.
    this->Texture->SetInputArrayToProcess(
      0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS, vtkDataSetAttributes::SCALARS);
    cellFlag = 0;
  }
  else
  {
    // Pass the scalar array choice to the texture.
    this->Texture->SetInputArrayToProcess(0, 0, 0,
      (cellFlag ? vtkDataObject::FIELD_ASSOCIATION_CELLS : vtkDataObject::FIELD_ASSOCIATION_POINTS),
      scalars->GetName());
  }
  return cellFlag;
}

//----------------------------------------------------------------------------
void vtkPVImageSliceMapper::RenderInternal(vtkRenderer* renderer, vtkActor* actor)
{
  vtkImageData* input = vtkImageData::SafeDownCast(this->GetInput());
  if (this->UpdateTime < input->GetMTime() || this->UpdateTime < this->MTime)
  {
    this->UpdateTime.Modified();
    int sliceDescription = 0;
    int inextent[6];
    int outextent[6];
    // we deliberately use whole extent here. So on processes where the slice is
    // not available, the vtkExtractVOI filter will simply yield an empty
    // output.
    int* wext =
      this->GetInputInformation(0, 0)->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
    memcpy(inextent, wext, 6 * sizeof(int));
    memcpy(outextent, inextent, sizeof(int) * 6);
    int numdims = ::vtkGetDataDimension(inextent);
    int dims[3];
    dims[0] = inextent[1] - inextent[0] + 1;
    dims[1] = inextent[3] - inextent[2] + 1;
    dims[2] = inextent[5] - inextent[4] + 1;

    // Based on the scalar mode, scalar array, scalar id,
    // we need to tell the vtkTexture to use the appropriate scalars.
    int cellFlag = this->SetupScalars(input);

    // Determine the VOI to extract:
    // * If the input image is 3D, then we respect the slice number and slice
    // direction the user recommended.
    // * If the input image is 2D, we simply show the input image slice.
    // * If the input image is 1D, we raise an error.
    if (numdims == 3)
    {
      int slice = this->Slice;
      // clamp the slice number at min val.
      slice = (slice < 0) ? 0 : slice;

      // if cell centered, then dimensions reduces by 1.
      int curdim = cellFlag ? (dims[this->SliceMode] - 1) : dims[this->SliceMode];

      // clamp the slice number at max val.
      slice = (slice >= curdim) ? curdim - 1 : slice;

      if (this->SliceMode == XY_PLANE) // XY plane
      {
        outextent[4] = outextent[5] = outextent[4] + slice;
        sliceDescription = VTK_XY_PLANE;
      }
      else if (this->SliceMode == YZ_PLANE) // YZ plane
      {
        outextent[0] = outextent[1] = outextent[0] + slice;
        sliceDescription = VTK_YZ_PLANE;
      }
      else if (this->SliceMode == XZ_PLANE) // XZ plane
      {
        outextent[2] = outextent[3] = outextent[2] + slice;
        sliceDescription = VTK_XZ_PLANE;
      }
    }
    else if (numdims == 2)
    {
      if (inextent[4] == inextent[5]) // XY plane
      {
        // nothing to change.
        sliceDescription = VTK_XY_PLANE;
      }
      else if (inextent[0] == inextent[1]) /// YZ plane
      {
        sliceDescription = VTK_YZ_PLANE;
      }
      else if (inextent[2] == inextent[3]) // XZ plane
      {
        sliceDescription = VTK_XZ_PLANE;
      }
    }
    else
    {
      vtkErrorMacro("Incorrect dimensionality.");
      return;
    }

    vtkNew<vtkImageData> clone;
    clone->ShallowCopy(input);

    vtkNew<vtkExtractVOI> extractVOI;
    extractVOI->SetVOI(outextent);
    extractVOI->SetInputData(clone.Get());
    extractVOI->Update();

    int evoi[6];
    extractVOI->GetOutput()->GetExtent(evoi);
    if (evoi[1] < evoi[0] && evoi[3] < evoi[2] && evoi[5] < evoi[4])
    {
      // if vtkExtractVOI did not produce a valid output, that means there's no
      // image slice to display.
      this->Texture->SetInputData(nullptr);
      return;
    }

    // TODO: Here we would have change the input scalars if the user asked us to.
    // The LUT can be simply passed to the vtkTexture. It can handle scalar
    // mapping.
    this->Texture->SetInputConnection(extractVOI->GetOutputPort());
    this->Texture->SetLookupTable(this->LookupTable);
    this->Texture->SetColorMode(this->ColorMode);

    // construct the plane extent in index space, then transform to
    // physical space when we assign to the polydata, below.
    // This correctly handles oriented images with a DirectionMatrix.
    double outputExtent[6];
    outputExtent[0] = inextent[0];
    outputExtent[1] = inextent[1];
    outputExtent[2] = inextent[2];
    outputExtent[3] = inextent[3];
    outputExtent[4] = inextent[4];
    outputExtent[5] = inextent[5];

    if (cellFlag)
    {
      // Structured bounds are point bounds. Shrink them to reflect cell
      // center bounds.
      // i.e move min bounds up by spacing/2 in that direction
      //     and move max bounds down by spacing/2 in that direction.
      // We are in index coords, so that's just 0.5 of a voxel
      for (int dir = 0; dir < 3; dir++)
      {
        double& min = outputExtent[2 * dir];
        double& max = outputExtent[2 * dir + 1];
        if (min + 1.0 <= max)
        {
          min += 0.5;
          max -= 0.5;
        }
        else
        {
          min = max = (min + 0.5);
        }
      }
    }

    const int* indices = nullptr;
    switch (sliceDescription)
    {
      case VTK_XY_PLANE:
        indices = XY_PLANE_QPOINTS_INDICES;
        if (this->UseXYPlane)
        {
          // z coord is clamped to zero below.
          indices = XY_PLANE_QPOINTS_INDICES_ORTHO;
        }
        break;

      case VTK_YZ_PLANE:
        indices = YZ_PLANE_QPOINTS_INDICES;
        if (this->UseXYPlane)
        {
          indices = YZ_PLANE_QPOINTS_INDICES_ORTHO;
        }
        break;

      case VTK_XZ_PLANE:
        indices = XZ_PLANE_QPOINTS_INDICES;
        if (this->UseXYPlane)
        {
          indices = XZ_PLANE_QPOINTS_INDICES_ORTHO;
        }
        break;
    }

    vtkPolyData* poly =
      vtkPolyDataMapper::SafeDownCast(this->PolyDataActor->GetMapper())->GetInput();
    vtkPoints* polyPoints = poly->GetPoints();

    for (int i = 0; i < 4; i++)
    {
      double outPt[3] = { outputExtent[indices[i * 3]], outputExtent[indices[3 * i + 1]],
        outputExtent[indices[3 * i + 2]] };
      // Using this transform respects the input image's DirectionMatrix, for oriented images.
      input->TransformContinuousIndexToPhysicalPoint(outPt, outPt);
      if (this->UseXYPlane)
      {
        outPt[2] = 0;
      }
      polyPoints->SetPoint(i, outPt);
    }
    polyPoints->Modified();
  }

  if (!this->Texture->GetInput())
  {
    return;
  }

  // copy information to the delegate
  this->PolyDataActor->vtkProp3D::ShallowCopy(actor);
  vtkInformation* info = actor->GetPropertyKeys();
  this->PolyDataActor->SetPropertyKeys(info);
  this->PolyDataActor->SetProperty(actor->GetProperty());

  // Render
  this->Texture->Render(renderer);
  this->PolyDataActor->GetMapper()->Render(renderer, this->PolyDataActor);
  this->Texture->PostRender(renderer);
}

//----------------------------------------------------------------------------
void vtkPVImageSliceMapper::ReleaseGraphicsResources(vtkWindow* win)
{
  this->Texture->ReleaseGraphicsResources(win);
  this->PolyDataActor->ReleaseGraphicsResources(win);
  this->Superclass::ReleaseGraphicsResources(win);
}

//----------------------------------------------------------------------------
void vtkPVImageSliceMapper::Render(vtkRenderer* ren, vtkActor* act)
{
  if (this->Static)
  {
    this->RenderPiece(ren, act);
  }
  vtkImageData* input = this->GetInput();
  if (!input)
  {
    vtkErrorMacro("Mapper has no vtkImageData input.");
    return;
  }

  vtkInformation* inInfo = this->GetInputInformation();

  int nPieces = this->NumberOfSubPieces * this->NumberOfPieces;
  for (int cc = 0; cc < this->NumberOfSubPieces; cc++)
  {
    int currentPiece = this->NumberOfSubPieces * this->Piece + cc;
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), currentPiece);
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), nPieces);
    inInfo->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), this->GhostLevel);
    this->RenderPiece(ren, act);
  }
}

//----------------------------------------------------------------------------
void vtkPVImageSliceMapper::SetInputData(vtkImageData* input)
{
  this->SetInputDataInternal(0, input);
}

//----------------------------------------------------------------------------
vtkImageData* vtkPVImageSliceMapper::GetInput()
{
  return vtkImageData::SafeDownCast(this->GetExecutive()->GetInputData(0, 0));
}

//----------------------------------------------------------------------------
void vtkPVImageSliceMapper::Update(int port)
{
  if (!this->Static)
  {
    int currentPiece, nPieces = this->NumberOfPieces;
    vtkImageData* input = this->GetInput();

    // If the estimated pipeline memory usage is larger than
    // the memory limit, break the current piece into sub-pieces.
    if (input)
    {
      vtkInformation* inInfo = this->GetInputInformation();
      currentPiece = this->NumberOfSubPieces * this->Piece;
      this->GetInputAlgorithm()->UpdateInformation();
      inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), currentPiece);
      inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
        this->NumberOfSubPieces * nPieces);
      inInfo->Set(
        vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), this->GhostLevel);
    }

    this->Superclass::Update(port);
  }
}

//----------------------------------------------------------------------------
double* vtkPVImageSliceMapper::GetBounds()
{
  static double bounds[6] = { -1.0, 1.0, -1.0, 1.0, -1.0, 1.0 };
  vtkImageData* input = this->GetInput();
  if (!input)
  {
    return bounds;
  }

  this->Update();
  input->GetBounds(this->Bounds);
  if (this->UseXYPlane)
  {
    // When using XY plane, the image will be in XY plane placed at the origin,
    // hence we adjust the bounds.
    if (this->Bounds[0] == this->Bounds[1])
    {
      this->Bounds[0] = this->Bounds[2];
      this->Bounds[1] = this->Bounds[3];
      this->Bounds[2] = this->Bounds[4];
      this->Bounds[3] = this->Bounds[5];
    }
    else if (this->Bounds[2] == this->Bounds[3])
    {
      this->Bounds[0] = this->Bounds[4];
      this->Bounds[1] = this->Bounds[5];
      this->Bounds[2] = this->Bounds[0];
      this->Bounds[3] = this->Bounds[1];
    }
    else if (this->Bounds[5] == this->Bounds[5])
    {
      // nothing to do.
    }
    // We check for SliceMode only if the input is not already 2D, since slice
    // mode is applicable only for 3D images.
    else if (this->SliceMode == YZ_PLANE)
    {
      this->Bounds[0] = this->Bounds[2];
      this->Bounds[1] = this->Bounds[3];
      this->Bounds[2] = this->Bounds[4];
      this->Bounds[3] = this->Bounds[5];
    }
    else if (this->SliceMode == XZ_PLANE)
    {
      this->Bounds[0] = this->Bounds[4];
      this->Bounds[1] = this->Bounds[5];
      this->Bounds[2] = this->Bounds[0];
      this->Bounds[3] = this->Bounds[1];
    }

    this->Bounds[4] = this->Bounds[5] = 0.0;
  }

  return this->Bounds;
}

//----------------------------------------------------------------------------
void vtkPVImageSliceMapper::ShallowCopy(vtkAbstractMapper* mapper)
{
  vtkPVImageSliceMapper* idmapper = vtkPVImageSliceMapper::SafeDownCast(mapper);
  if (idmapper)
  {
    this->SetInputData(idmapper->GetInput());
    this->SetGhostLevel(idmapper->GetGhostLevel());
    this->SetNumberOfPieces(idmapper->GetNumberOfPieces());
    this->SetNumberOfSubPieces(idmapper->GetNumberOfSubPieces());
  }

  this->Superclass::ShallowCopy(mapper);
}

//----------------------------------------------------------------------------
int vtkPVImageSliceMapper::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVImageSliceMapper::RenderPiece(vtkRenderer* ren, vtkActor* actor)
{
  vtkImageData* input = this->GetInput();
  //
  // make sure that we've been properly initialized
  //
  if (ren->GetRenderWindow()->CheckAbortStatus())
  {
    return;
  }
  if (input == nullptr)
  {
    vtkErrorMacro(<< "No input!");
    return;
  }
  else
  {
    this->InvokeEvent(vtkCommand::StartEvent, nullptr);
    if (!this->Static)
    {
      this->Update();
    }
    this->InvokeEvent(vtkCommand::EndEvent, nullptr);

    vtkIdType numPts = input->GetNumberOfPoints();
    if (numPts == 0)
    {
      vtkDebugMacro(<< "No points!");
      return;
    }
  }
  // make sure our window is current
  ren->GetRenderWindow()->MakeCurrent();
  this->TimeToDraw = 0.0;
  this->RenderInternal(ren, actor);

  // If the timer is not accurate enough, set it to a small
  // time so that it is not zero
  if (this->TimeToDraw == 0.0)
  {
    this->TimeToDraw = 0.0001;
  }

  this->UpdateProgress(1.0);
}

//----------------------------------------------------------------------------
void vtkPVImageSliceMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Piece : " << this->Piece << endl;
  os << indent << "NumberOfPieces : " << this->NumberOfPieces << endl;
  os << indent << "GhostLevel: " << this->GhostLevel << endl;
  os << indent << "Number of sub pieces: " << this->NumberOfSubPieces << endl;
}
