/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVRenderSlave.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-1999 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/

#include "vtkPVRenderSlave.h"
#include "vtkMesaRenderWindow.h"
#include "vtkMesaRenderer.h"
#include "vtkObjectFactory.h"



//----------------------------------------------------------------------------
vtkPVRenderSlave* vtkPVRenderSlave::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVRenderSlave");
  if(ret)
    {
    return (vtkPVRenderSlave*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPVRenderSlave;
}


int vtkPVRenderSlaveCommand(ClientData cd, Tcl_Interp *interp,
                             int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVRenderSlave::vtkPVRenderSlave()
{
  vtkMesaRenderWindow *mesaRenWindow;
  vtkMesaRenderer *mesaRenderer;

  mesaRenderWindow = vtkMesaRenderWindow::New();
  mesaRenderWindow->SetOffScreenRenderer(1);
  mesaRenderer = vtkMesaRenderer::New();
  mesaRenderWindow->AddRenderer(mesaRenderer);

  this->CommandFunction = vtkPVRenderSlaveCommand;
  this->PVSlave = NULL;
  this->RenderWindow = mesaRenderWindow;
  this->Renderer = mesaRenderer;
}

//----------------------------------------------------------------------------
vtkPVRenderSlave::~vtkPVRenderSlave()
{
  this->Interactor->Delete();
  this->Interactor = NULL;
  this->SetInteractorStyle(NULL);
}

//----------------------------------------------------------------------------
void vtkPVRenderSlave::Render()
{
  this->RenderWindow->Render();
  .....
}

//----------------------------------------------------------------------------
void vtkPVRenderSlave::SetInteractorStyle(vtkInteractorStyle *style)
{
  if (this->Interactor)
    {
    this->Interactor->SetRenderWindow(this->GetRenderer()->GetRenderWindow());
    this->Interactor->SetInteractorStyle(style);
    }
  if (this->InteractorStyle)
    {
    this->InteractorStyle->UnRegister(this);
    this->InteractorStyle = NULL;
    }
  if (style)
    {
    this->InteractorStyle = style;
    style->Register(this);
    }
} 

//----------------------------------------------------------------------------
void vtkPVRenderSlave::Create(vtkKWApplication *app, char *args)
{
  if (this->Application)
    {
    vtkErrorMacro("RenderView already created");
    return;
    }

  this->vtkKWRenderView::Create(app, args);

  // Styles need motion events.
  this->Script("bind %s <Motion> {%s MotionCallback %%x %%y}", 
               this->VTKWidget->GetWidgetName(), this->GetTclName());


  // Here we are going to create a single slave renderer in another process.
  // (As a test)
  vtkPVApplication *pvApp = (vtkPVApplication *)(app);
  pvApp->RemoteScript(0, "vtkPVRenderSlave RenderSlave", NULL, 0);
  // The global "Slave" was setup when the process was initiated.
  pvApp->RemoteScript(0, "RenderSlave SetPVSlave Slave", NULL, 0);
   
}

//----------------------------------------------------------------------------
// Called by a binding, so I must flip y.
void vtkPVRenderSlave::MotionCallback(int x, int y)
{
  int *size = this->GetRenderer()->GetSize();
  y = size[1] - y;

  if (this->InteractorStyle)
    {
    this->InteractorStyle->OnMouseMove(0, 0, x, y);
    }
}


//----------------------------------------------------------------------------
void vtkPVRenderSlave::AButtonPress(int num, int x, int y)
{
  int *size = this->GetRenderer()->GetSize();
  y = size[1] - y;

  if (this->InteractorStyle)
    {
    if (num == 1)
      {
      this->InteractorStyle->OnLeftButtonDown(0, 0, x, y);
      }
    if (num == 2)
      {
      this->InteractorStyle->OnMiddleButtonDown(0, 0, x, y);
      }
    if (num == 3)
      {
      this->InteractorStyle->OnRightButtonDown(0, 0, x, y);
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVRenderSlave::AButtonRelease(int num, int x, int y)
{
  int *size = this->GetRenderer()->GetSize();
  y = size[1] - y;

  if (this->InteractorStyle)
    {
    if (num == 1)
      {
      this->InteractorStyle->OnLeftButtonUp(0, 0, x, y);
      }
    if (num == 2)
      {
      this->InteractorStyle->OnMiddleButtonUp(0, 0, x, y);
      }
    if (num == 3)
      {
      this->InteractorStyle->OnRightButtonUp(0, 0, x, y);
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVRenderSlave::Button1Motion(int x, int y)
{
  int *size = this->GetRenderer()->GetSize();
  y = size[1] - y;

  if (this->InteractorStyle)
    {
    this->InteractorStyle->OnMouseMove(0, 0, x, y);
    }
}

//----------------------------------------------------------------------------
void vtkPVRenderSlave::Button2Motion(int x, int y)
{
  int *size = this->GetRenderer()->GetSize();
  y = size[1] - y;

  if (this->InteractorStyle)
    {
    this->InteractorStyle->OnMouseMove(0, 0, x, y);
    }
}

//----------------------------------------------------------------------------
void vtkPVRenderSlave::Button3Motion(int x, int y)
{
  int *size = this->GetRenderer()->GetSize();
  y = size[1] - y;

  if (this->InteractorStyle)
    {
    this->InteractorStyle->OnMouseMove(0, 0, x, y);
    }
}

//----------------------------------------------------------------------------
void vtkPVRenderSlave::AKeyPress(char key, int x, int y)
{
  x = y;

  if (this->InteractorStyle)
    {
    this->InteractorStyle->OnChar(0, 0, key, 1);
    }
}

