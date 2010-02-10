/*=========================================================================

  Program:   ParaView
  Module:    vtkCaveRenderManager.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
#include "vtkCaveRenderManager.h"

#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkIceTContext.h"
#include "vtkIceTRenderer.h"
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkMPIController.h"
#include "vtkMPI.h"
#include "vtkMultiProcessStream.h"
#include "vtkObjectFactory.h"
#include "vtkPerspectiveTransform.h"
#include "vtkPKdTree.h"
#include "vtkProcessModule.h"
#include "vtkRendererCollection.h"
#include "vtkRenderWindow.h"
#include "vtkTimerLog.h"
#include "vtkUnsignedCharArray.h"

#include <GL/ice-t.h>

#include "vtkgl.h"
#include "assert.h"
#include <vtkstd/algorithm>

#include "vtkTransform.h"

// ******************************************************************
// Callback commands.
// ******************************************************************

//******************************************************************
// vtkCaveRenderManager implementation.
//******************************************************************

vtkCxxRevisionMacro(vtkCaveRenderManager, "1.4");
vtkStandardNewMacro(vtkCaveRenderManager);

vtkCaveRenderManager::vtkCaveRenderManager()
{
  this->NumberOfDisplays = 0;
  this->Displays= 0;
  this->SetNumberOfDisplays(1);

  this->DisplayOrigin[0] = 0.0;
  this->DisplayOrigin[1] = 0.0;
  this->DisplayOrigin[2] = 0.0;
  this->DisplayOrigin[3] = 1.0;
  this->DisplayX[0] = 0.0;
  this->DisplayX[1] = 0.0;
  this->DisplayX[2] = 0.0;
  this->DisplayX[3] = 1.0;
  this->DisplayY[0] = 0.0;
  this->DisplayY[1] = 0.0;
  this->DisplayY[2] = 0.0;
  this->DisplayY[3] = 1.0;
  // Reload the controller so that we make an ICE-T context.
  this->Superclass::SetController(NULL);
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//-------------------------------------------------------------------------

vtkCaveRenderManager::~vtkCaveRenderManager()
{
  this->SetController(NULL);
  this->SetNumberOfDisplays(0);
}
//-------------------------------------------------------------------------
// Room camera is a camera in room coordinates that points at the display.
// Client camera is the camera on the client.  The out camera is the 
// combination of the two used for the final cave display.
// It is the room camera transformed by the world camera.
void vtkCaveRenderManager::ComputeCamera(vtkCamera* cam)
{
  int idx;
  int display = this->Controller->GetLocalProcessId();
  double* displayOrigin = this->Displays[display];
  double* displayX = &(this->Displays[display][4]);
  double* displayY = &(this->Displays[display][8]);
  // pos is the user position
  double pos[4];
  //cam->GetPosition(pos);
  pos[0] = 0.;
  pos[1] = 0.; 
  pos[2] = 0.;
  pos[3] = 1.;

  // Use the camera here  tempoarily to get the client view transform.
  //cam->SetFocalPoint(originalCam->GetFocalPoint());
  //cam->SetPosition(info->ClientCameraPosition);
  //cam->SetViewUp(info->ClientCameraViewUp);
  // Create a transform from the client camera.
  vtkTransform* trans = cam->GetViewTransformObject();
  // The displays are defined in camera coordinates.
  // We want to convert them to world coordinates.
  trans->Inverse();

  // Apply the transform on the local display info.
  double p[4];
  double o[4];
  double x[4];
  double y[4];
  trans->MultiplyPoint(pos, p);
  /*
  trans->MultiplyPoint(displayOrigin, o);
  trans->MultiplyPoint(displayX, x);
  trans->MultiplyPoint(displayY, y);
  */
  trans->MultiplyPoint(this->DisplayOrigin, o);
  trans->MultiplyPoint(this->DisplayX, x);
  trans->MultiplyPoint(this->DisplayY, y);
  // Handle homogeneous coordinates.
  for (idx = 0; idx < 3; ++idx)
    {
    p[idx] = p[idx] / p[3];
    o[idx] = o[idx] / o[3];
    x[idx] = x[idx] / x[3];
    y[idx] = y[idx] / y[3];
    }

  // Now compute the camera.
  float vn[3];
  float ox[3];
  float oy[3];
  float cp[3];
  float center[3];
  float offset[3];
  float xOffset, yOffset;
  float dist;
  float height;
  float width;
  float viewAngle;
  float tmp;

  // Compute the view plane normal.
  for ( idx = 0; idx < 3; ++idx)
    {
    ox[idx] = x[idx] - o[idx];
    oy[idx] = y[idx] - o[idx];
    center[idx] = o[idx] + 0.5*(ox[idx] + oy[idx]);
    cp[idx] = p[idx] - center[idx];
    }
  vtkMath::Cross(ox, oy, vn);
  vtkMath::Normalize(vn);
  // Compute distance to plane.
  dist = vtkMath::Dot(vn,cp);
  // Compute width and height of the window.
  width = sqrt(ox[0]*ox[0] + ox[1]*ox[1] + ox[2]*ox[2]);
  height = sqrt(oy[0]*oy[0] + oy[1]*oy[1] + oy[2]*oy[2]);

  // Point the camera orthogonal toward the plane.
  cam->SetPosition(p[0], p[1], p[2]);
  cam->SetFocalPoint(p[0]-vn[0], p[1]-vn[1], p[2]-vn[2]);
  cam->SetViewUp(oy[0], oy[1], oy[2]);
  
  // Compute view angle.
  viewAngle = atan(height/(2.0*dist)) * 360.0 / 3.1415926;
  cam->SetViewAngle(viewAngle);

  // Compute the shear/offset vector (focal point to window center).
  offset[0] = center[0] - (p[0]-dist*vn[0]);
  offset[1] = center[1] - (p[1]-dist*vn[1]);
  offset[2] = center[2] - (p[2]-dist*vn[2]);

  // Compute the normalized x and y components of shear offset.
  tmp = sqrt(ox[0]*ox[0] + ox[1]*ox[1] + ox[2]*ox[2]);
  xOffset = vtkMath::Dot(offset, ox) / (tmp * tmp); 
  tmp = sqrt(oy[0]*oy[0] + oy[1]*oy[1] + oy[2]*oy[2]);
  yOffset = vtkMath::Dot(offset, oy) / (tmp * tmp); 

  // Off angle positioning of window.
  cam->SetWindowCenter(2*xOffset, 2*yOffset);
}

//-------------------------------------------------------------------------

vtkRenderer *vtkCaveRenderManager::MakeRenderer()
{
  return vtkIceTRenderer::New();
}

//-------------------------------------------------------------------------

void vtkCaveRenderManager::SetController(vtkMultiProcessController *controller)
{
  vtkDebugMacro("SetController to " << controller);

  if (controller == this->Controller)
    {
    return;
    }

  if (controller != NULL)
    {
    vtkCommunicator *communicator = controller->GetCommunicator();
    if (!communicator || (!communicator->IsA("vtkMPICommunicator")))
      {
      vtkErrorMacro("vtkCaveRenderManager parallel compositor currently works only with an MPI communicator.");
      return;
      }
    }

  this->Superclass::SetController(controller);
}

//-----------------------------------------------------------------------------

void vtkCaveRenderManager::SetRenderWindow(vtkRenderWindow *renwin)
{
  if (this->RenderWindow == renwin)
    {
    return;
    }

  this->Superclass::SetRenderWindow(renwin);

  this->ContextDirty = 1;
}

//-----------------------------------------------------------------------------
void vtkCaveRenderManager::SetNumberOfDisplays(int numberOfDisplays)
{
  if (numberOfDisplays == this->NumberOfDisplays)
    {
    return;
    }
  double** newDisplays = 0;
  if (numberOfDisplays > 0)
    {
    newDisplays = new double*[numberOfDisplays];
    for (int i = 0; i < numberOfDisplays; ++i)
      {
      newDisplays[i] = new double[12];
      if (i < this->NumberOfDisplays)
        {
        memcpy(newDisplays[i], this->Displays[i], 12 * sizeof(double));
        }
      else
        {
        newDisplays[i][0] = -1.;
        newDisplays[i][1] = -1.;
        newDisplays[i][2] = -1.;
        newDisplays[i][3] = 1.0;

        newDisplays[i][4] = 1.0;
        newDisplays[i][5] = -1.0;
        newDisplays[i][6] = -1.0;
        newDisplays[i][7] = 1.0;
        
        newDisplays[i][8] = -1.0;
        newDisplays[i][9] = 1.0;
        newDisplays[i][10] = -1.0;
        newDisplays[i][11] = 1.0;
        }
      }
    }
  for (int i = 0; i < this->NumberOfDisplays; ++i)
    {
    delete [] this->Displays[i];
    }
  delete [] this->Displays;
  this->Displays = newDisplays;

  this->NumberOfDisplays = numberOfDisplays;
  //this->Modified();
}

//-----------------------------------------------------------------------------
void vtkCaveRenderManager::SetDisplay(double idx, double origin0, double origin1, double origin2, 
                              double x0, double x1, double x2,
                              double y0, double y1, double y2)
{
  double origin[3];
  double x[3];
  double y[3];
  origin[0] = origin0;
  origin[1] = origin1;
  origin[2] = origin2;
  x[0] = x0;
  x[1] = x1;
  x[2] = x2;
  y[0] = y0;
  y[1] = y1;
  y[2] = y2;
  this->DefineDisplay(static_cast<int>(idx), origin, x, y);
}

//-------------------------------------------------------------------------
void vtkCaveRenderManager::DefineDisplay(int idx, double origin[3],
                                         double x[3], double y[3])
{
  if (idx >= this->NumberOfDisplays)
    {
    vtkErrorMacro("idx is too high !");
    return;
    }
  memcpy(&this->Displays[idx][0], origin, 3 * sizeof(double));
  memcpy(&this->Displays[idx][4], x, 3 * sizeof(double));
  memcpy(&this->Displays[idx][8], y, 3 * sizeof(double));
  if (idx == this->Controller->GetLocalProcessId())
    {
    memcpy(this->DisplayOrigin, origin, 3 * sizeof(double));
    memcpy(this->DisplayX, x, 3 * sizeof(double));
    memcpy(this->DisplayY, y, 3 * sizeof(double));
    }
  // this->Displays will be synchronized with the others processes in processWindow
  this->Modified();
}
//-----------------------------------------------------------------------------
void vtkCaveRenderManager::CollectWindowInformation(vtkMultiProcessStream& stream)
{
  vtkDebugMacro("Sending Window Information");

  this->Superclass::CollectWindowInformation(stream);

  // insert the tag to ensure we reading back the correct information.
  stream << vtkProcessModule::IceTWinInfo;

  stream << this->NumberOfDisplays;
  for (int x = 0; x < this->NumberOfDisplays; x++)
    {
    for (int i = 0; i < 12 ; ++i)
      {
      stream << this->Displays[x][i];
      }
    }  
  stream << vtkProcessModule::IceTWinInfo;
}

//-----------------------------------------------------------------------------

bool vtkCaveRenderManager::ProcessWindowInformation(vtkMultiProcessStream& stream)
{
  vtkDebugMacro("Receiving Window Information");

  if (!this->Superclass::ProcessWindowInformation(stream))
    {
    return false;
    }

  int tag;
  stream >> tag;
  if (tag != vtkProcessModule::IceTWinInfo)
    {
    vtkErrorMacro("Incorrect tag received. Aborting for debugging purposes.");
    return false;
    }

  int numDisplays;
  stream >> numDisplays;
  this->SetNumberOfDisplays(numDisplays);

  for (int x = 0; x < numDisplays; x++)
    {
    for (int i = 0; i < 12 ; ++i)
      {
      stream >> this->Displays[x][i];
      }
    if (x == this->Controller->GetLocalProcessId())
      {
      memcpy(this->DisplayOrigin, &this->Displays[x][0], 4*sizeof(double));
      memcpy(this->DisplayX, &this->Displays[x][4], 4*sizeof(double));
      memcpy(this->DisplayY, &this->Displays[x][8], 4*sizeof(double));
      }
    }
  stream >> tag;
  if (tag != vtkProcessModule::IceTWinInfo)
    {
    vtkErrorMacro("Incorrect tag received. Aborting for debugging purposes.");
    return false;
    }
  return true;
}

//-----------------------------------------------------------------------------
void vtkCaveRenderManager::CollectRendererInformation(vtkRenderer *_ren,
  vtkMultiProcessStream& stream)
{
  vtkDebugMacro("Sending renderer information for " << _ren);

  this->Superclass::CollectRendererInformation(_ren, stream);

  vtkIceTRenderer *ren = vtkIceTRenderer::SafeDownCast(_ren);
  if (!ren)
    {
    return;
    }

  stream << ren->GetStrategy()
         << ren->GetComposeOperation();
}

//-----------------------------------------------------------------------------
bool vtkCaveRenderManager::ProcessRendererInformation(vtkRenderer *_ren,
  vtkMultiProcessStream& stream)
{
  vtkDebugMacro("Receiving renderer information for " << _ren);

  if (!this->Superclass::ProcessRendererInformation(_ren, stream))
    {
    return false;
    }

  vtkIceTRenderer *ren = vtkIceTRenderer::SafeDownCast(_ren);
  if (ren) 
    {
    int strategy;
    int compose_operation;
    stream >> strategy >> compose_operation;
    ren->SetStrategy(strategy);
    ren->SetComposeOperation(compose_operation);
    }
  return true;
}

//-----------------------------------------------------------------------------
void vtkCaveRenderManager::PreRenderProcessing()
{
  vtkRendererCollection *rens;
  vtkRenderer* ren;
  vtkCamera *cam = 0;
  //vtkLightCollection *lc;
  //vtkLight *light;
  vtkRenderWindow* renWin = this->RenderWindow;

  // Delay swapping buffers until all processes are finished.
  //if (this->Controller)
  if (this->UseBackBuffer)
    {
    renWin->SwapBuffersOff();  
    }

  // Synchronize
  rens = renWin->GetRenderers();
  rens->InitTraversal();
  // NOTE:  We are now receiving first!!!!!  
  // This will probably cause a bug based on the following comment
  // about getting the active camera.
  // "We put this before receive because we want the pipeline to be
  // updated the first time if the camera does not exist and we want
  // it to happen before we block in receive"
  ren = rens->GetNextItem();
  if (ren == NULL)
    {
    vtkErrorMacro("Renderer mismatch.");
    }
  else
    {
    //lc = ren->GetLights();
    //lc->InitTraversal();
    //light = lc->GetNextItem();
    // Setup tile independent stuff
    cam = ren->GetActiveCamera();
    this->ComputeCamera(cam);
    /*
    if (light)
      {
      light->SetPosition(info->LightPosition);
      light->SetFocalPoint(info->LightFocalPoint);
      }
      */
    //ren->SetBackground(info->Background);
    ren->ResetCameraClippingRange();
    }

  if (this->UseBackBuffer)
    {
    this->RenderWindow->SwapBuffersOff();
    }
}

//-----------------------------------------------------------------------------

void vtkCaveRenderManager::PostRenderProcessing()
{
  vtkDebugMacro("PostRenderProcessing");

  this->Controller->Barrier();

  // Swap buffers here.
  if (this->UseBackBuffer)
    {
    this->RenderWindow->SwapBuffersOn();
    }
  this->RenderWindow->Frame();
}
//-----------------------------------------------------------------------------

void vtkCaveRenderManager::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
