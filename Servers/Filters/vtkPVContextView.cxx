/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVContextView.h"

#include "vtkContextView.h"
#include "vtkExtractVOI.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkProcessModule.h"
#include "vtkPVOptions.h"
#include "vtkPVSynchronizedRenderWindows.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkSmartPointer.h"
#include "vtkTileDisplayHelper.h"
#include "vtkTilesHelper.h"
#include "vtkTimerLog.h"
#include "vtkUnsignedCharArray.h"
#include "vtkWindowToImageFilter.h"

//----------------------------------------------------------------------------
vtkPVContextView::vtkPVContextView()
{
  this->RenderWindow = this->SynchronizedWindows->NewRenderWindow();
  this->ContextView = vtkContextView::New();
  this->ContextView->SetRenderWindow(this->RenderWindow);
}

//----------------------------------------------------------------------------
vtkPVContextView::~vtkPVContextView()
{
  vtkTileDisplayHelper::GetInstance()->EraseTile(this);

  this->RenderWindow->Delete();
  this->ContextView->Delete();
}

//----------------------------------------------------------------------------
void vtkPVContextView::Initialize(unsigned int id)
{
  this->SynchronizedWindows->AddRenderWindow(id, this->RenderWindow);
  this->SynchronizedWindows->AddRenderer(id, this->ContextView->GetRenderer());
  this->Superclass::Initialize(id);
}

//----------------------------------------------------------------------------
void vtkPVContextView::StillRender()
{
  vtkTimerLog::MarkStartEvent("Still Render");
  this->Render(false);
  vtkTimerLog::MarkEndEvent("Still Render");
}

//----------------------------------------------------------------------------
void vtkPVContextView::InteractiveRender()
{
  vtkTimerLog::MarkStartEvent("Interactive Render");
  this->Render(true);
  vtkTimerLog::MarkEndEvent("Interactive Render");
}

//----------------------------------------------------------------------------
void vtkPVContextView::Render(bool interactive)
{
  if (!interactive)
    {
    // Update all representations.
    // This should update mostly just the inputs to the representations, and maybe
    // the internal geometry filter.
    this->Update();
    }

  // Since currently we only support client-side rendering, we disable render
  // synchronization for charts among all processes.
  this->SynchronizedWindows->SetEnabled(false);

  // Call Render() on local render window only on the client (or root node in
  // batch mode).
  if (this->SynchronizedWindows->GetLocalProcessIsDriver())
    {
    this->ContextView->Render();
    if (this->InTileDisplayMode())
      {
      this->SendImageToRenderServers();
      }
    }
  else if (this->InTileDisplayMode())
    {
    // We turn EraseOff so that the image we never overwrite the image pasted
    // from the client-side.
    this->ContextView->GetRenderer()->EraseOff();
    this->ReceiveImageToFromClient();
    vtkTileDisplayHelper::GetInstance()->FlushTiles(this);
    }
}

#include <math.h>
int ComputeMagnification(const int full_size[2], int window_size[2])
{
  int magnification = 1;

  // If fullsize > viewsize, then magnification is involved.
  int temp = ceil(full_size[0]/static_cast<double>(window_size[0]));
  magnification = (temp> magnification)? temp: magnification;

  temp = ceil(full_size[1]/static_cast<double>(window_size[1]));
  magnification = (temp > magnification)? temp : magnification;
  window_size[0] = full_size[0]/magnification;
  window_size[1] = full_size[1]/magnification;
  return magnification;
}

//----------------------------------------------------------------------------
void vtkPVContextView::SendImageToRenderServers()
{
  int size[2];
  this->SynchronizedWindows->GetClientServerController()->Receive(
    size, 2, 1, 238903);
  int actual_size[2], prev_size[2];
  actual_size[0] = this->GetRenderWindow()->GetSize()[0];
  actual_size[1] = this->GetRenderWindow()->GetSize()[1];
  prev_size[0] = actual_size[0];
  prev_size[1] = actual_size[1];

  int magnification = ComputeMagnification(size, actual_size);
  this->RenderWindow->SetSize(actual_size);
  vtkWindowToImageFilter* w2i = vtkWindowToImageFilter::New();
  w2i->SetInput(this->GetRenderWindow());
  w2i->SetMagnification(magnification);
  w2i->ReadFrontBufferOff();
  w2i->ShouldRerenderOff();
  this->RenderWindow->SwapBuffersOff();
  w2i->Update();
  this->RenderWindow->SwapBuffersOn();
  this->RenderWindow->SetSize(prev_size);
  this->SynchronizedWindows->BroadcastToRenderServer(w2i->GetOutput());
  w2i->Delete();
}

#define MIN(x,y) (x < y? x : y)
//----------------------------------------------------------------------------
void vtkPVContextView::ReceiveImageToFromClient()
{
  double viewport[4];
  this->ContextView->GetRenderer()->GetViewport(viewport);

  int size[2];
  size[0] = this->GetRenderWindow()->GetSize()[0];
  size[1] = this->GetRenderWindow()->GetSize()[1];
  size[0] *= (viewport[2]-viewport[0]);
  size[1] *= (viewport[3]-viewport[1]);
  if (this->SynchronizedWindows->GetClientServerController())
    {
    this->SynchronizedWindows->GetClientServerController()->Send(
      size, 2, 1, 238903);
    }

  vtkImageData* image = vtkImageData::New();
  this->SynchronizedWindows->BroadcastToRenderServer(image);

  int tile_dims[2], tile_mullions[2];
  this->SynchronizedWindows->GetTileDisplayParameters(tile_dims, tile_mullions);

  double tile_viewport[4];
  this->GetRenderWindow()->GetTileViewport(tile_viewport);

  int image_dims[3];
  image->GetDimensions(image_dims);

  // Extract sub-section from that image based on what will be project on the
  // current tile.
  vtkExtractVOI* voi = vtkExtractVOI::New();
  voi->SetInput(image);
  voi->SetVOI(
    MIN(1.0, (tile_viewport[0]-viewport[0]) / (viewport[2] - viewport[0]))*image_dims[0],
    MIN(1.0, (tile_viewport[2]-viewport[0]) / (viewport[2] - viewport[0]))*image_dims[0],
    MIN(1.0, (tile_viewport[1]-viewport[1]) / (viewport[3] - viewport[1]))*image_dims[1],
    MIN(1.0, (tile_viewport[3]-viewport[1]) / (viewport[3] - viewport[1]))*image_dims[1],
    0, 0);
  voi->Update();
  image->ShallowCopy(voi->GetOutput());
  voi->Delete();

  double physical_viewport[4];
  vtkSmartPointer<vtkTilesHelper> tilesHelper = vtkSmartPointer<vtkTilesHelper>::New();
  tilesHelper->SetTileDimensions(tile_dims);
  tilesHelper->SetTileMullions(tile_mullions);
  tilesHelper->SetTileWindowSize(this->GetRenderWindow()->GetActualSize());
  tilesHelper->GetPhysicalViewport(viewport,
    vtkMultiProcessController::GetGlobalController()->GetLocalProcessId(),
    physical_viewport);

  vtkSynchronizedRenderers::vtkRawImage tile;
  tile.Initialize(image->GetDimensions()[0],
    image->GetDimensions()[1],
    vtkUnsignedCharArray::SafeDownCast(image->GetPointData()->GetScalars()));
  vtkTileDisplayHelper::GetInstance()->SetTile(this,
    physical_viewport,
    this->ContextView->GetRenderer(), tile);
  image->Delete();
}

//----------------------------------------------------------------------------
void vtkPVContextView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
