/*=========================================================================

  Program:   ParaView
  Module:    vtkPVGenericRenderWindowInteractor.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVGenericRenderWindowInteractor.h"

#include "vtkRenderer.h"
#include "vtkPVRenderView.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRendererCollection.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVGenericRenderWindowInteractor);
vtkCxxRevisionMacro(vtkPVGenericRenderWindowInteractor, "1.17");
vtkCxxSetObjectMacro(vtkPVGenericRenderWindowInteractor,Renderer,vtkRenderer);

//----------------------------------------------------------------------------
vtkPVGenericRenderWindowInteractor::vtkPVGenericRenderWindowInteractor()
{
  this->PVRenderView = NULL;
  this->Renderer = NULL;
  this->ReductionFactor = 1;
  this->InteractiveRenderEnabled = 0;
}

//----------------------------------------------------------------------------
vtkPVGenericRenderWindowInteractor::~vtkPVGenericRenderWindowInteractor()
{
  this->SetPVRenderView(NULL);
  this->SetRenderer(NULL);
}

//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::SetPVRenderView(vtkPVRenderView *view)
{
  if (this->PVRenderView != view)
    {
    // to avoid circular references
    this->PVRenderView = view;
    if (this->PVRenderView != NULL)
      {
      this->SetRenderWindow(this->PVRenderView->GetRenderWindow());
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::SetMoveEventInformationFlipY(
  int x, int y)
{
  this->SetEventInformationFlipY(x, y, this->ControlKey, this->ShiftKey,
                                 this->KeyCode, this->RepeatCount,
                                 this->KeySym);
}

//----------------------------------------------------------------------------
vtkRenderer* vtkPVGenericRenderWindowInteractor::FindPokedRenderer(int,int)
{
  if (this->Renderer == NULL)
    {
    vtkErrorMacro("Renderer has not been set.");
    }

  return this->Renderer;
}


//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::Render()
{
  if ( this->PVRenderView == NULL || this->RenderWindow == NULL)
    { // The case for interactors on the satellite processes.
    return;
    }

  // This should fix the problem of the plane widget render 
  if (this->InteractiveRenderEnabled)
    {
    this->PVRenderView->Render();
    }
  else
    {
    this->PVRenderView->EventuallyRender();
    }
}


//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::SetInteractiveRenderEnabled(int val)
{
  if (this->InteractiveRenderEnabled == val)
    {
    return;
    }
  this->Modified();
  this->InteractiveRenderEnabled = val;
  if (val == 0 && this->PVRenderView)
    {
    this->PVRenderView->EventuallyRender();
    }
}



// Special methods for forwarding events to satellite processes.
// The take care of the reduction factor by comparing 
// renderer size with render window size. 


//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::SatelliteLeftPress(int x, int y, 
                                                     int control, int shift)
{
  int *winSize;
  int *renSize;
  winSize = this->RenderWindow->GetSize();
  vtkRendererCollection *rc = this->RenderWindow->GetRenderers();
  rc->InitTraversal();
  vtkRenderer *aren = rc->GetNextItem();
  renSize = aren->GetSize();
  int reductionFactor = this->CalculateReductionFactor(winSize[1], renSize[1]);
  x = (int)((float)x / (float)(reductionFactor));
  y = (int)((float)(winSize[1]-y) / (float)(reductionFactor));
  this->SetEventInformation(x, y, control, shift);
  if (reductionFactor != this->ReductionFactor)
    {
    this->LastEventPosition[0] = this->LastEventPosition[0] 
                                   * this->ReductionFactor / reductionFactor;
    this->LastEventPosition[1] = this->LastEventPosition[1] 
                                   * this->ReductionFactor / reductionFactor;
    this->Size[0] = renSize[0];
    this->Size[1] = renSize[1];
    this->ReductionFactor = reductionFactor;
    }
  this->LeftButtonPressEvent();
}

//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::SatelliteMiddlePress(int x, int y, 
                                                     int control, int shift)
{
  int *winSize;
  int *renSize;
  winSize = this->RenderWindow->GetSize();
  vtkRendererCollection *rc = this->RenderWindow->GetRenderers();
  rc->InitTraversal();
  vtkRenderer *aren = rc->GetNextItem();
  renSize = aren->GetSize();
  int reductionFactor = this->CalculateReductionFactor(winSize[1], renSize[1]);
  x = (int)((float)x / (float)(reductionFactor));
  y = (int)((float)(winSize[1]-y) / (float)(reductionFactor));
  this->SetEventInformation(x, y, control, shift);
  if (reductionFactor != this->ReductionFactor)
    {
    this->LastEventPosition[0] = this->LastEventPosition[0] 
                                   * this->ReductionFactor / reductionFactor;
    this->LastEventPosition[1] = this->LastEventPosition[1] 
                                   * this->ReductionFactor / reductionFactor;
    this->Size[0] = renSize[0];
    this->Size[1] = renSize[1];
    this->ReductionFactor = reductionFactor;
    }
  this->MiddleButtonPressEvent();
}

//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::SatelliteRightPress(int x, int y, 
                                                     int control, int shift)
{
  int *winSize;
  int *renSize;
  winSize = this->RenderWindow->GetSize();
  vtkRendererCollection *rc = this->RenderWindow->GetRenderers();
  rc->InitTraversal();
  vtkRenderer *aren = rc->GetNextItem();
  renSize = aren->GetSize();
  int reductionFactor = this->CalculateReductionFactor(winSize[1], renSize[1]);
  x = (int)((float)x / (float)(reductionFactor));
  y = (int)((float)(winSize[1]-y) / (float)(reductionFactor));
  this->SetEventInformation(x, y, control, shift);
  if (reductionFactor != this->ReductionFactor)
    {
    this->LastEventPosition[0] = this->LastEventPosition[0] 
                                   * this->ReductionFactor / reductionFactor;
    this->LastEventPosition[1] = this->LastEventPosition[1] 
                                   * this->ReductionFactor / reductionFactor;
    this->Size[0] = renSize[0];
    this->Size[1] = renSize[1];
    this->ReductionFactor = reductionFactor;
    }
  this->RightButtonPressEvent();
}

//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::SatelliteLeftRelease(int x, int y, 
                                                       int control, int shift)
{
  int *winSize;
  int *renSize;
  winSize = this->RenderWindow->GetSize();
  vtkRendererCollection *rc = this->RenderWindow->GetRenderers();
  rc->InitTraversal();
  vtkRenderer *aren = rc->GetNextItem();
  renSize = aren->GetSize();
  int reductionFactor = this->CalculateReductionFactor(winSize[1], renSize[1]);
  x = (int)((float)x / (float)(reductionFactor));
  y = (int)((float)(winSize[1]-y) / (float)(reductionFactor));
  this->SetEventInformation(x, y, control, shift);
  if (reductionFactor != this->ReductionFactor)
    {
    this->LastEventPosition[0] = this->LastEventPosition[0] 
                                   * this->ReductionFactor / reductionFactor;
    this->LastEventPosition[1] = this->LastEventPosition[1] 
                                   * this->ReductionFactor / reductionFactor;
    this->Size[0] = renSize[0];
    this->Size[1] = renSize[1];
    this->ReductionFactor = reductionFactor;
    }
  this->LeftButtonReleaseEvent();
}

//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::SatelliteMiddleRelease(int x, int y, 
                                                        int control, int shift)
{
  int *winSize;
  int *renSize;
  winSize = this->RenderWindow->GetSize();
  vtkRendererCollection *rc = this->RenderWindow->GetRenderers();
  rc->InitTraversal();
  vtkRenderer *aren = rc->GetNextItem();
  renSize = aren->GetSize();
  int reductionFactor = this->CalculateReductionFactor(winSize[1], renSize[1]);
  x = (int)((float)x / (float)(reductionFactor));
  y = (int)((float)(winSize[1]-y) / (float)(reductionFactor));
  this->SetEventInformation(x, y, control, shift);
  if (reductionFactor != this->ReductionFactor)
    {
    this->LastEventPosition[0] = this->LastEventPosition[0] 
                                   * this->ReductionFactor / reductionFactor;
    this->LastEventPosition[1] = this->LastEventPosition[1] 
                                   * this->ReductionFactor / reductionFactor;
    this->Size[0] = renSize[0];
    this->Size[1] = renSize[1];
    this->ReductionFactor = reductionFactor;
    }
  this->MiddleButtonReleaseEvent();
}

//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::SatelliteRightRelease(int x, int y, 
                                                        int control, int shift)
{
  int *winSize;
  int *renSize;
  winSize = this->RenderWindow->GetSize();
  vtkRendererCollection *rc = this->RenderWindow->GetRenderers();
  rc->InitTraversal();
  vtkRenderer *aren = rc->GetNextItem();
  renSize = aren->GetSize();
  int reductionFactor = this->CalculateReductionFactor(winSize[1], renSize[1]);
  x = (int)((float)x / (float)(reductionFactor));
  y = (int)((float)(winSize[1]-y) / (float)(reductionFactor));
  this->SetEventInformation(x, y, control, shift);
  if (reductionFactor != this->ReductionFactor)
    {
    this->LastEventPosition[0] = this->LastEventPosition[0] 
                                   * this->ReductionFactor / reductionFactor;
    this->LastEventPosition[1] = this->LastEventPosition[1] 
                                   * this->ReductionFactor / reductionFactor;
    this->Size[0] = renSize[0];
    this->Size[1] = renSize[1];
    this->ReductionFactor = reductionFactor;
    }
  this->RightButtonReleaseEvent();
}

//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::SatelliteMove(int x, int y) 
{
  int* winSize;
  int* renSize;
  winSize = this->RenderWindow->GetSize();
  vtkRendererCollection *rc = this->RenderWindow->GetRenderers();
  rc->InitTraversal();
  vtkRenderer *aren = rc->GetNextItem();
  renSize = aren->GetSize(); 
  int reductionFactor = this->CalculateReductionFactor(winSize[1], renSize[1]);
  x = (int)((float)x / (float)(reductionFactor));
  y = (int)((float)(winSize[1]-y) / (float)(reductionFactor));
  this->SetEventInformation(x, y, this->ControlKey, this->ShiftKey,
                                 this->KeyCode, this->RepeatCount,
                                 this->KeySym);
  if (reductionFactor != this->ReductionFactor)
    {
    this->LastEventPosition[0] = this->LastEventPosition[0] 
                                   * this->ReductionFactor / reductionFactor;
    this->LastEventPosition[1] = this->LastEventPosition[1] 
                                   * this->ReductionFactor / reductionFactor;
    this->Size[0] = renSize[0];
    this->Size[1] = renSize[1];
    this->ReductionFactor = reductionFactor;
    }
  this->MouseMoveEvent();
}


//----------------------------------------------------------------------------
int vtkPVGenericRenderWindowInteractor::CalculateReductionFactor(int winSize1,
                                                                 int renSize1)
{
  if(winSize1 > 0 && renSize1 > 0)
    {
    return (int)(0.5 + ((float)winSize1/(float)renSize1));
    }
  else
    {
    return 1;
    }
}


//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "PVRenderView: " << this->GetPVRenderView() << endl;
  os << indent << "InteractiveRenderEnabled: " 
     << this->InteractiveRenderEnabled << endl;
  os << indent << "Renderer: " << this->Renderer << endl;
}
