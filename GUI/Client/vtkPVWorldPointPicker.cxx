/*=========================================================================

  Program:   ParaView
  Module:    vtkPVWorldPointPicker.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVWorldPointPicker.h"

#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVRenderModule.h"
#include "vtkRenderer.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVWorldPointPicker);
vtkCxxRevisionMacro(vtkPVWorldPointPicker, "1.10.12.1");

vtkCxxSetObjectMacro(vtkPVWorldPointPicker, RenderModule, vtkPVRenderModule);

//----------------------------------------------------------------------------
vtkPVWorldPointPicker::vtkPVWorldPointPicker()
{
  this->RenderModule = NULL;
}

//----------------------------------------------------------------------------
vtkPVWorldPointPicker::~vtkPVWorldPointPicker()
{
  this->SetRenderModule(NULL);
}


//----------------------------------------------------------------------------
// Perform pick operation with selection point provided. The z location
// is recovered from the zBuffer. Always returns 0 since no actors are picked.
int vtkPVWorldPointPicker::Pick(double selectionX, double selectionY, 
                                double selectionZ, vtkRenderer *renderer)
{
  vtkCamera *camera;
  double cameraFP[4];
  double display[3], *world;
  double *displayCoord;
  double z;

  if (this->RenderModule == NULL)
    {
    return vtkWorldPointPicker::Pick(selectionX, selectionY, selectionZ, renderer);
    }

  // Initialize the picking process
  this->Initialize();
  this->Renderer = renderer;
  this->SelectionPoint[0] = selectionX;
  this->SelectionPoint[1] = selectionY;
  this->SelectionPoint[2] = selectionZ;

  // Invoke start pick method if defined
  this->InvokeEvent(vtkCommand::StartPickEvent,NULL);

  z = this->RenderModule->GetZBufferValue ((int) selectionX, (int) selectionY);
  
  // if z is 1.0, we assume the user has picked a point on the
  // screen that has not been rendered into. Use the camera's focal
  // point for the z value. The test value .999999 has to be used
  // instead of 1.0 because for some reason our SGI Infinite Reality
  // engine won't return a 1.0 from the zbuffer
  if (z < 0.999999)
    {
    selectionZ = z;
    vtkDebugMacro(<< " z from zBuffer: " << selectionZ);
    }
  else
    {
    // Get camera focal point and position. Convert to display (screen) 
    // coordinates. We need a depth value for z-buffer.
    camera = renderer->GetActiveCamera();
    camera->GetFocalPoint(cameraFP); cameraFP[3] = 1.0;

    renderer->SetWorldPoint(cameraFP);
    renderer->WorldToDisplay();
    displayCoord = renderer->GetDisplayPoint();
    selectionZ = displayCoord[2];
    vtkDebugMacro(<< "computed z from focal point: " << selectionZ);
    }

  // now convert the display point to world coordinates
  display[0] = selectionX;
  display[1] = selectionY;
  display[2] = selectionZ;

  renderer->SetDisplayPoint (display);
  renderer->DisplayToWorld ();
  world = renderer->GetWorldPoint ();
  
  for (int i=0; i < 3; i++) 
    {
    this->PickPosition[i] = world[i] / world[3];
    }

  // Invoke end pick method if defined
  this->InvokeEvent(vtkCommand::EndPickEvent,NULL);

  return 0;
}



//----------------------------------------------------------------------------
void vtkPVWorldPointPicker::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkWorldPointPicker::PrintSelf(os,indent);

  os << indent << "RenderModule: " << this->RenderModule << endl;
}

