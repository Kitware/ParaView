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

#include "vtkCaveRenderManager.h"
#include "vtkMath.h"
#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkTimerLog.h"
#include "vtkLight.h"
#include "vtkLightCollection.h"
#include "vtkMultiProcessController.h"
#include "vtkSocketController.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkToolkits.h"
#include "vtkUnsignedCharArray.h"
#include "vtkFloatArray.h"
#include "vtkTransform.h"

#ifdef _WIN32
#include "vtkWin32OpenGLRenderWindow.h"
#endif

#ifdef VTK_USE_MPI
 #include <mpi.h>
#endif

vtkCxxRevisionMacro(vtkCaveRenderManager, "1.1");
vtkStandardNewMacro(vtkCaveRenderManager);

// Structures to communicate render info.
// Marshaling is easier if we use all floats. (24)
class vtkPVCaveClientInfo 
{
public:
  vtkPVCaveClientInfo();
  double ClientCameraPosition[3];
  double ClientCameraFocalPoint[3];
  double ClientCameraViewUp[3];
  double LightPosition[3];
  double LightFocalPoint[3];
  double Background[3];
  // Room coordinates.
  double UserPosition[4];
};
vtkPVCaveClientInfo::vtkPVCaveClientInfo()
{
  this->ClientCameraPosition[0] = 0.0;               
  this->ClientCameraPosition[1] = 0.0;               
  this->ClientCameraPosition[2] = 0.0;               
  this->ClientCameraFocalPoint[0] = 0.0;    
  this->ClientCameraFocalPoint[1] = 0.0;    
  this->ClientCameraFocalPoint[2] = 0.0;    
  this->ClientCameraViewUp[0] = 0.0;  
  this->ClientCameraViewUp[1] = 0.0;  
  this->ClientCameraViewUp[2] = 0.0;  
  this->LightPosition[0] = 0.0;
  this->LightPosition[1] = 0.0;
  this->LightPosition[2] = 0.0;
  this->LightFocalPoint[0] = 0.0;
  this->LightFocalPoint[1] = 0.0;
  this->LightFocalPoint[2] = 0.0;
  this->Background[0] = 0.0;
  this->Background[1] = 0.0;
  this->Background[2] = 0.0;
  this->UserPosition[0] = 0.0;               
  this->UserPosition[1] = 0.0;               
  this->UserPosition[2] = 0.0; 
  this->UserPosition[3] = 1.0; 
}
// Structures to communicate render info.
// Marshaling is easier if we use all floats. (24)
class vtkPVCaveDisplayInfo 
{
public:
  vtkPVCaveDisplayInfo();
  double DisplayIndex;
  // Room coordinates.
  double DisplayOrigin[3];
  double DisplayX[3];
  double DisplayY[3];
};

vtkPVCaveDisplayInfo::vtkPVCaveDisplayInfo()
{
  this->DisplayOrigin[0] = 0.0;               
  this->DisplayOrigin[1] = 0.0;               
  this->DisplayOrigin[2] = 0.0;
  this->DisplayX[0] = 0.0;
  this->DisplayX[1] = 0.0;
  this->DisplayX[2] = 0.0;     
  this->DisplayY[0] = 0.0;
  this->DisplayY[1] = 0.0;
  this->DisplayY[2] = 0.0;
}



//-------------------------------------------------------------------------
vtkCaveRenderManager::vtkCaveRenderManager()
{
  this->ClientFlag = 0;
  this->SocketController = NULL;

  this->StartTag = this->EndTag = 0;

  this->DisplayOrigin[0] = 0;
  this->DisplayOrigin[1] = 0;
  this->DisplayOrigin[2] = 0;
  this->DisplayOrigin[3] = 1.0;
  this->DisplayX[0] = 1.0;
  this->DisplayX[1] = 0.0;
  this->DisplayX[2] = 0.0;
  this->DisplayX[3] = 1.0;
  this->DisplayY[0] = 0.0;
  this->DisplayY[1] = 1.0;
  this->DisplayY[2] = 0.0;
  this->DisplayY[3] = 1.0;
}

  
//-------------------------------------------------------------------------
vtkCaveRenderManager::~vtkCaveRenderManager()
{
  this->SetSocketController(0);
}




//-------------------------------------------------------------------------
// Called by the render window start event. 
float vtkCaveRenderManager::GetZBufferValue(int x, int y)
{
  float z;
  float *pz;

  if (this->RenderWindow == NULL)
    {
    vtkErrorMacro("Missing render window.");
    return 0.5;
    }
  
  pz = this->RenderWindow->GetZbufferData(x, y, x, y);
  z = *pz;
  delete [] pz;
  return z;  
}



//==================== CallbackCommand and RMI functions ====================


//-------------------------------------------------------------------------
// Called by the render window start event. 
void vtkCaveRenderManagerDefineDisplayRMI(void *localArg, 
                                          void *, int, int)
{
  vtkCaveRenderManager *self = (vtkCaveRenderManager *)localArg;

  self->DefineDisplayRMI();
}

//-------------------------------------------------------------------------
// Called by the render window start event. 
void vtkCaveRenderManagerClientStartRender(vtkObject *caller,
                                 unsigned long vtkNotUsed(event), 
                                 void *clientData, void *)
{
  vtkCaveRenderManager *self = (vtkCaveRenderManager *)clientData;

  if (caller != self->GetRenderWindow())
    { // Sanity check.
    vtkGenericWarningMacro("Caller mismatch.");
    return;
    }

  self->ClientStartRender();
}

//-------------------------------------------------------------------------
// Called by the render window start event. 
void vtkCaveRenderManagerClientEndRender(vtkObject *caller,
                                 unsigned long vtkNotUsed(event), 
                                 void *clientData, void *)
{
  (void)caller;
  vtkCaveRenderManager *self = (vtkCaveRenderManager *)clientData;

  if (caller != self->GetRenderWindow())
    { // Sanity check.
    vtkGenericWarningMacro("Caller mismatch.");
    return;
    }

  self->ClientEndRender();
}


typedef void (*vtkRMIFunctionType)(void *localArg, 
                                   void *remoteArg, int remoteArgLength, 
                                   int remoteProcessId);

//-------------------------------------------------------------------------
void vtkCaveRenderManagerRootStartRenderRMI(void *localArg, 
                                           void *, int, int)
{
  vtkCaveRenderManager *self = (vtkCaveRenderManager *)localArg;
  vtkMultiProcessController *controller = self->GetSocketController();
  vtkPVCaveClientInfo info;  

  controller->Receive((double*)(&info), 
                     sizeof(vtkPVCaveClientInfo)/sizeof(double), 1, 
                     vtkCaveRenderManager::INFO_TAG);
  self->RootStartRenderRMI(&info);
}

//-------------------------------------------------------------------------
void vtkCaveRenderManagerSatelliteStartRenderRMI(void *localArg, 
                                                 void *, int, int)
{
  vtkCaveRenderManager *self = (vtkCaveRenderManager *)localArg;
  self->SatelliteStartRenderRMI();
}



//==================== end of callback and RMI functions ====================


//-------------------------------------------------------------------------
// Room camera is a camera in room coordinates that points at the display.
// Client camera is the camera on the client.  The out camera is the 
// combination of the two used for the final cave display.
// It is the room camera transformed by the world camera.
void vtkCaveRenderManager::ComputeCamera(vtkPVCaveClientInfo *info,
                                         vtkCamera* cam)
{
  int idx;

  // Use the camera here  tempoarily to get the client view transform.
  cam->SetFocalPoint(info->ClientCameraFocalPoint);
  cam->SetPosition(info->ClientCameraPosition);
  cam->SetViewUp(info->ClientCameraViewUp);
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
  trans->MultiplyPoint(info->UserPosition, p);
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
void vtkCaveRenderManager::DefineDisplay(int idx, double origin[3],
                                         double x[3], double y[3])
{
  vtkPVCaveDisplayInfo info;

  info.DisplayIndex = (double)idx;
  info.DisplayOrigin[0] = origin[0];
  info.DisplayOrigin[1] = origin[1];
  info.DisplayOrigin[2] = origin[2];
  info.DisplayX[0] = x[0];
  info.DisplayX[1] = x[1];
  info.DisplayX[2] = x[2];
  info.DisplayY[0] = y[0];
  info.DisplayY[1] = y[1];
  info.DisplayY[2] = y[2];

  this->SocketController->TriggerRMI(1, NULL, 0, 
                   vtkCaveRenderManager::DEFINE_DISPLAY_RMI_TAG);
  this->SocketController->Send((double*)(&info), 
                     sizeof(vtkPVCaveDisplayInfo)/sizeof(double), 
                     1, vtkCaveRenderManager::DEFINE_DISPLAY_INFO_TAG);
}

//-------------------------------------------------------------------------
// Only called on "client".
void vtkCaveRenderManager::ClientStartRender()
{
  vtkPVCaveClientInfo info;
  int numProcs;
  vtkRendererCollection *rens;
  vtkRenderer* ren;
  vtkCamera *cam;
  vtkLightCollection *lc;
  vtkLight *light;
  
  vtkDebugMacro("StartRender");
  // Make sure they all swp buffers at the same time.
  this->RenderWindow->SwapBuffersOff();

  rens = this->RenderWindow->GetRenderers();
  numProcs = this->Controller->GetNumberOfProcesses();

  // Synchronize cameras
  rens->InitTraversal();
  // Assume only one renderer.
  ren = rens->GetNextItem();
  cam = ren->GetActiveCamera();
  lc = ren->GetLights();
  lc->InitTraversal();
  light = lc->GetNextItem();
  cam->GetPosition(info.ClientCameraPosition);
  cam->GetFocalPoint(info.ClientCameraFocalPoint);
  cam->GetViewUp(info.ClientCameraViewUp);
  if (light)
    {
    light->GetPosition(info.LightPosition);
    light->GetFocalPoint(info.LightFocalPoint);
    }
  ren->GetBackground(info.Background);

  // Trigger the satellite processes to start their render routine.  
  if (this->SocketController)
    { // client... Send to root
    this->SocketController->TriggerRMI(1, NULL, 0, 
                     vtkCaveRenderManager::ROOT_RENDER_RMI_TAG);
    this->SocketController->Send((double*)(&info), 
                     sizeof(vtkPVCaveClientInfo)/sizeof(double), 1, 
                     vtkCaveRenderManager::INFO_TAG);
    }
  else
    {
    // Client is also root.  Call directly.
    this->RootStartRenderRMI(&info);   
    }
}


//-------------------------------------------------------------------------
// I am using the same RMI for both root and satellites.
void vtkCaveRenderManager::DefineDisplayRMI()
{
  int myId = this->Controller->GetLocalProcessId();
  vtkPVCaveDisplayInfo info; 
  int idx; 

  if (myId == 0)
    {
    this->SocketController->Receive((double*)(&info), 
                           sizeof(vtkPVCaveDisplayInfo)/sizeof(double), 1, 
                           vtkCaveRenderManager::DEFINE_DISPLAY_INFO_TAG);
    if (info.DisplayIndex != 0)
      { // Pass display info to appropriate satellite.
      this->Controller->TriggerRMI(1, NULL, 0, 
                       vtkCaveRenderManager::DEFINE_DISPLAY_RMI_TAG);
      this->Controller->Send((double*)(&info), 
                       sizeof(vtkPVCaveDisplayInfo)/sizeof(double), 
                       static_cast<int>(info.DisplayIndex), 
                       vtkCaveRenderManager::DEFINE_DISPLAY_INFO_TAG);
      return;
      }
    }
  else
    {
    // We are on a satellite. Receive info from root.
    this->Controller->Receive((double*)(&info), 
                      sizeof(vtkPVCaveDisplayInfo)/sizeof(double), 0, 
                      vtkCaveRenderManager::DEFINE_DISPLAY_INFO_TAG);
    if (info.DisplayIndex != myId)
      { 
      vtkErrorMacro("Wrong display.");
      return;
      }
    }

  for (idx = 0; idx < 3; ++idx)
    {
    this->DisplayOrigin[idx] = info.DisplayOrigin[idx];
    this->DisplayX[idx] = info.DisplayX[idx];
    this->DisplayY[idx] = info.DisplayY[idx];
    }
  this->DisplayOrigin[3] = 1.0;
  this->DisplayX[3] = 1.0;
  this->DisplayY[3] = 1.0;
}

//-------------------------------------------------------------------------
// Only called on "root".
void vtkCaveRenderManager::RootStartRenderRMI(vtkPVCaveClientInfo *info)
{
  int id, numProcs;

  if (this->Controller)
    {
    numProcs = this->Controller->GetNumberOfProcesses();
    }
  else
    {
    numProcs = 1;
    }

  // Every process (except "client") gets to participate.  
  for (id = 1; id < numProcs; ++id)
    {
    this->Controller->TriggerRMI(id, NULL, 0, 
                     vtkCaveRenderManager::SATELLITE_RENDER_RMI_TAG);
    this->Controller->Send((double*)(info), 
                     sizeof(vtkPVCaveClientInfo)/sizeof(double), id,
                     vtkCaveRenderManager::INFO_TAG);
    }
  if ( this->SocketController)
    { // Root is not client, it participates also.
    this->InternalSatelliteStartRender(info);
    }
}

//-------------------------------------------------------------------------
void vtkCaveRenderManager::SatelliteStartRenderRMI()
{
  vtkPVCaveClientInfo info;

  this->Controller->Receive((double*)(&info), 
                            sizeof(vtkPVCaveClientInfo)/sizeof(double), 0, 
                            vtkCaveRenderManager::INFO_TAG);
  this->InternalSatelliteStartRender(&info);
}

//-------------------------------------------------------------------------
void vtkCaveRenderManager::InternalSatelliteStartRender(vtkPVCaveClientInfo *info)
{
  vtkRendererCollection *rens;
  vtkRenderer* ren;
  vtkCamera *cam = 0;
  vtkLightCollection *lc;
  vtkLight *light;
  vtkRenderWindow* renWin = this->RenderWindow;

  // Delay swapping buffers untill all processes are finished.
  if (this->Controller)
    {
    renWin->SwapBuffersOff();  
    }

  // Synchronize
  rens = renWin->GetRenderers();
  rens->InitTraversal();
  // NOTE:  We are now receiving first!!!!!  
  // This will probably cause a bug based on the folloing comment
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
    lc = ren->GetLights();
    lc->InitTraversal();
    light = lc->GetNextItem();
    // Setup tile independent stuff
    cam = ren->GetActiveCamera();
    this->ComputeCamera(info, cam);
    if (light)
      {
      light->SetPosition(info->LightPosition);
      light->SetFocalPoint(info->LightFocalPoint);
      }
    ren->SetBackground(info->Background);
    ren->ResetCameraClippingRange();
    }

    this->RenderWindow->Render();

  // Synchronize here to have all procs swap buffers at the same time.
  if (this->Controller)
    {
    this->Controller->Barrier();
    }
  if (this->SocketController)
    {
    this->SocketController->Barrier();
    // Socket barrier is not implemented.
    // Just send a message to synchronize.
    int dummyMessage = 10;
    this->SocketController->Send(&dummyMessage,1, 1, 12323);
    }

  // Force swap buffers here.
  renWin->SwapBuffersOn();  
  renWin->Frame();
}


//-------------------------------------------------------------------------
// Only client needs start and end render callbacks.
void vtkCaveRenderManager::SetRenderWindow(vtkRenderWindow *renWin)
{
  int clientFlag = 0;

  if (this->RenderWindow == renWin)
    {
    return;
    }
  this->Modified();

  if (this->ClientFlag)
    {
    clientFlag = 1;
    }

  if (this->RenderWindow)
    {
    // Remove all of the observers.
    if (clientFlag)
      {
      this->RenderWindow->RemoveObserver(this->StartTag);
      this->RenderWindow->RemoveObserver(this->EndTag);
      }
    // Delete the reference.
    this->RenderWindow->UnRegister(this);
    this->RenderWindow =  NULL;
    }
  if (renWin)
    {
    renWin->Register(this);
    this->RenderWindow = renWin;
    if (clientFlag)
      {
      vtkCallbackCommand *cbc;
      
      cbc= vtkCallbackCommand::New();
      cbc->SetCallback(vtkCaveRenderManagerClientStartRender);
      cbc->SetClientData((void*)this);
      // renWin will delete the cbc when the observer is removed.
      this->StartTag = renWin->AddObserver(vtkCommand::StartEvent,cbc);
      cbc->Delete();
        
      cbc = vtkCallbackCommand::New();
      cbc->SetCallback(vtkCaveRenderManagerClientEndRender);
      cbc->SetClientData((void*)this);
      // renWin will delete the cbc when the observer is removed.
      this->EndTag = renWin->AddObserver(vtkCommand::EndEvent,cbc);
      cbc->Delete();
      }
    else
      {
      renWin->FullScreenOn();
      }
    }
}


//-------------------------------------------------------------------------
void vtkCaveRenderManager::SetController(vtkMultiProcessController *mpc)
{
  if (this->Controller == mpc)
    {
    return;
    }
  if (mpc)
    {
    mpc->Register(this);
    }
  if (this->Controller)
    {
    this->Controller->UnRegister(this);
    }
  this->Controller = mpc;
}



//-------------------------------------------------------------------------
void vtkCaveRenderManager::SetSocketController(vtkSocketController *mpc)
{
  if (this->SocketController == mpc)
    {
    return;
    }
  if (mpc)
    {
    mpc->Register(this);
    }
  if (this->SocketController)
    {
    this->SocketController->UnRegister(this);
    }
  this->SocketController = mpc;
}



//-------------------------------------------------------------------------
// This is only called in the satellite processes (not 0).
void vtkCaveRenderManager::InitializeRMIs()
{
  // Adding RMIs to processes that do not need them is harmless ...
  if (this->SocketController)
    {
    this->SocketController->AddRMI(vtkCaveRenderManagerRootStartRenderRMI, (void*)this, 
                                   vtkCaveRenderManager::ROOT_RENDER_RMI_TAG); 
    this->SocketController->AddRMI(vtkCaveRenderManagerDefineDisplayRMI, (void*)this, 
                                   vtkCaveRenderManager::DEFINE_DISPLAY_RMI_TAG); 
    }

  if (this->Controller)
    {
    this->Controller->AddRMI(vtkCaveRenderManagerSatelliteStartRenderRMI, (void*)this, 
                             vtkCaveRenderManager::SATELLITE_RENDER_RMI_TAG); 
    this->Controller->AddRMI(vtkCaveRenderManagerDefineDisplayRMI, (void*)this, 
                             vtkCaveRenderManager::DEFINE_DISPLAY_RMI_TAG); 
    }
}


//-------------------------------------------------------------------------
void vtkCaveRenderManager::ClientEndRender()
{
  vtkRenderWindow* renWin = this->RenderWindow;
  
  if (this->SocketController)
    {
    this->SocketController->Barrier();
    // Since socket barrier is not implemented,
    // just receive a message to synchronize.
    int dummyMessage;
    this->SocketController->Receive(&dummyMessage,1, 1, 12323);
    }

  if (renWin)
    {
    renWin->SwapBuffersOn();  
    renWin->Frame();
    }
}


//----------------------------------------------------------------------------
void vtkCaveRenderManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  
  os << indent << "ClientFlag: " << this->ClientFlag << endl;

  if ( this->RenderWindow )
    {
    os << indent << "RenderWindow: " << this->RenderWindow << "\n";
    }
  else
    {
    os << indent << "RenderWindow: (none)\n";
    }

  os << indent << "Controller: (" << this->Controller << ")\n"; 
  os << indent << "SocketController: (" << this->SocketController << ")\n"; 
}



