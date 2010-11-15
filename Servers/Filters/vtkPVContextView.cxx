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
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkProcessModule.h"
#include "vtkPVOptions.h"
#include "vtkPVSynchronizedRenderWindows.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkTileDisplayHelper.h"
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
    //this->GetRenderWindow()->Render();
    if (this->InTileDisplayMode())
      {
      this->SendImageToRenderServers();
      }
    }
  else if (this->InTileDisplayMode())
    {
    this->ReceiveImageToFromClient();
    }
}

//----------------------------------------------------------------------------
void vtkPVContextView::SendImageToRenderServers()
{
  vtkWindowToImageFilter* w2i = vtkWindowToImageFilter::New();
  w2i->SetInput(this->GetRenderWindow());
  w2i->SetMagnification(/*FIXME:magnification*/ 1 );
  w2i->ReadFrontBufferOff();
  w2i->ShouldRerenderOff();
  w2i->Update();

  this->SynchronizedWindows->BroadcastToRenderServer(w2i->GetOutput());
}

//----------------------------------------------------------------------------
void vtkPVContextView::ReceiveImageToFromClient()
{
  vtkImageData* image = vtkImageData::New();
  this->SynchronizedWindows->BroadcastToRenderServer(image);

  vtkSynchronizedRenderers::vtkRawImage tile;
  tile.Initialize(image->GetDimensions()[0],
    image->GetDimensions()[1],
    vtkUnsignedCharArray::SafeDownCast(image->GetPointData()->GetScalars()));
  double viewport[4];
  this->ContextView->GetRenderer()->GetViewport(viewport);
  vtkTileDisplayHelper::GetInstance()->SetTile(this,
    viewport,
    this->ContextView->GetRenderer(), tile);
}

//----------------------------------------------------------------------------
void vtkPVContextView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
