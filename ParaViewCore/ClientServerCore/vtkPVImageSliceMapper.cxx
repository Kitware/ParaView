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

#include "vtkObjectFactory.h"
#include "vtkInformation.h"
#include "vtkTexturePainter.h"
#include "vtkImageData.h"
#include "vtkCommand.h"
#include "vtkScalarsToColorsPainter.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkExecutive.h"
#include "vtkDataArray.h"
//-----------------------------------------------------------------------------
class vtkPVImageSliceMapper::vtkObserver : public vtkCommand
{
public:
  static vtkObserver* New()
    { return new vtkObserver; }

  virtual void Execute(vtkObject* caller, unsigned long event, void*)
    {
    vtkPainter* p = vtkPainter::SafeDownCast(caller);
    if (this->Target && p && event == vtkCommand::ProgressEvent)
      {
      this->Target->UpdateProgress(p->GetProgress());
      }
    }
  vtkObserver()
    {
    this->Target = 0;
    }
  vtkPVImageSliceMapper* Target;
};

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

  this->Observer = vtkObserver::New();
  this->Observer->Target = this;
  this->Painter = 0;
  
  this->PainterInformation = vtkInformation::New();
  vtkTexturePainter* painter = vtkTexturePainter::New();
  this->SetPainter(painter);
  painter->Delete();
}

//----------------------------------------------------------------------------
vtkPVImageSliceMapper::~vtkPVImageSliceMapper()
{
  this->SetPainter(NULL);

  this->Observer->Target = 0;
  this->Observer->Delete();
  this->PainterInformation->Delete();
}

//-----------------------------------------------------------------------------
void vtkPVImageSliceMapper::SetPainter(vtkPainter* p)
{
  if (this->Painter)
    {
    this->Painter->RemoveObservers(vtkCommand::ProgressEvent, this->Observer);
    this->Painter->SetInformation(0);
    }
  vtkSetObjectBodyMacro(Painter, vtkPainter, p);
   if (this->Painter)
    {
    this->Painter->AddObserver(vtkCommand::ProgressEvent, this->Observer);
    this->Painter->SetInformation(this->PainterInformation);
    }
}

//----------------------------------------------------------------------------
void vtkPVImageSliceMapper::ReleaseGraphicsResources (vtkWindow *win)
{
  this->Painter->ReleaseGraphicsResources(win);
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

  int nPieces = this->NumberOfSubPieces* this->NumberOfPieces;
  for (int cc=0; cc < this->NumberOfSubPieces; cc++)
    {
    int currentPiece = this->NumberOfSubPieces * this->Piece + cc;
    vtkStreamingDemandDrivenPipeline::SetUpdateExtent(this->GetInputInformation(),
      currentPiece, nPieces, this->GhostLevel);
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
  // Set the whole extent on the painter because it needs it internally
  // and it has no access to the pipeline information.
  vtkTexturePainter* ptr = vtkTexturePainter::SafeDownCast(this->GetPainter());

  if (!this->Static)
    {
    int currentPiece, nPieces = this->NumberOfPieces;
    vtkImageData* input = this->GetInput();

    // If the estimated pipeline memory usage is larger than
    // the memory limit, break the current piece into sub-pieces.
    if (input)
      {
      this->GetInputAlgorithm()->UpdateInformation();
      currentPiece = this->NumberOfSubPieces * this->Piece;
      vtkStreamingDemandDrivenPipeline::SetUpdateExtent(
        this->GetInputInformation(),
        currentPiece, this->NumberOfSubPieces*nPieces, this->GhostLevel);
      }

    this->Superclass::Update(port);
    }


  int *wext = this->GetInputInformation(0, 0)->Get(
    vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
  if (wext)
    {
    ptr->SetWholeExtent(wext);
    }
}


//----------------------------------------------------------------------------
double* vtkPVImageSliceMapper::GetBounds()
{
  static double bounds[6] = {-1.0, 1.0, -1.0, 1.0, -1.0, 1.0};
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
int vtkPVImageSliceMapper::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVImageSliceMapper::UpdatePainterInformation()
{
  vtkInformation* info = this->PainterInformation;
  info->Set(vtkPainter::STATIC_DATA(), this->Static);
  
  // tell which array to color with.
  if (this->ScalarMode == VTK_SCALAR_MODE_USE_FIELD_DATA)
    {
    vtkErrorMacro("Field data coloring is not supported.");
    this->ScalarMode = VTK_SCALAR_MODE_DEFAULT;
    }

  if (this->ArrayAccessMode == VTK_GET_ARRAY_BY_ID)
    {
    info->Remove(vtkTexturePainter::SCALAR_ARRAY_NAME());
    info->Set(vtkTexturePainter::SCALAR_ARRAY_INDEX(), this->ArrayId);
    }
  else
    {
    info->Remove(vtkTexturePainter::SCALAR_ARRAY_INDEX());
    info->Set(vtkTexturePainter::SCALAR_ARRAY_NAME(), this->ArrayName);
    }
  info->Set(vtkTexturePainter::SCALAR_MODE(), this->ScalarMode);
  info->Set(vtkTexturePainter::LOOKUP_TABLE(), this->LookupTable);
  info->Set(vtkTexturePainter::USE_XY_PLANE(), this->UseXYPlane);

  // tell is we should map unsiged chars thorough LUT.
  info->Set(vtkTexturePainter::MAP_SCALARS(), 
    (this->ColorMode == VTK_COLOR_MODE_MAP_SCALARS)? 1 : 0);

  // tell information about the slice.
  info->Set(vtkTexturePainter::SLICE(), this->Slice);
  switch(this->SliceMode)
    {
  case YZ_PLANE:
    info->Set(vtkTexturePainter::SLICE_MODE(), vtkTexturePainter::YZ_PLANE);
    break;

  case XZ_PLANE:
    info->Set(vtkTexturePainter::SLICE_MODE(), vtkTexturePainter::XZ_PLANE);
    break;

  case XY_PLANE:
    info->Set(vtkTexturePainter::SLICE_MODE(), vtkTexturePainter::XY_PLANE);
    break;
    }
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
  if ( input == NULL ) 
    {
    vtkErrorMacro(<< "No input!");
    return;
    }
  else
    {
    this->InvokeEvent(vtkCommand::StartEvent,NULL);
    if (!this->Static)
      {
      this->Update();
      }
    this->InvokeEvent(vtkCommand::EndEvent,NULL);

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
  if (this->Painter)
    {
    // Update Painter information if obsolete.
    if (this->PainterInformationUpdateTime < this->GetMTime())
      {
      this->UpdatePainterInformation();
      this->PainterInformationUpdateTime.Modified();
      }
    // Pass polydata if changed.
    if (this->Painter->GetInput() != input)
      {
      this->Painter->SetInput(input);
      }
    this->Painter->Render(ren, actor, 0xff,this->ForceCompileOnly==1);
    this->TimeToDraw = this->Painter->GetTimeToDraw();
    }

  // If the timer is not accurate enough, set it to a small
  // time so that it is not zero
  if ( this->TimeToDraw == 0.0 )
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
