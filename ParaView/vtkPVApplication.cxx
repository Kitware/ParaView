/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVApplication.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-2000 Kitware Inc. 469 Clifton Corporate Parkway,
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
#include "vtkPVApplication.h"
#include "vtkPVWindow.h"
#include "vtkKWDialog.h"
#include "vtkKWWindowCollection.h"

#include "vtkMultiProcessController.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkKWMessageDialog.h"
#include "vtkTimerLog.h"
#include "vtkObjectFactory.h"
#include "vtkTclUtil.h"
#include "vtkPolyDataMapper.h"
#include "vtkKWResetViewButton.h"
#include "vtkKWFlyButton.h"
#include "vtkKWRotateViewButton.h"
#include "vtkKWTranslateViewButton.h"
#include "vtkKWPickCenterButton.h"
#include "vtkOutputWindow.h"



extern "C" int Vtktkrenderwidget_Init(Tcl_Interp *interp);
extern "C" int Vtkkwparaviewtcl_Init(Tcl_Interp *interp);
//extern "C" int Vtkparalleltcl_Init(Tcl_Interp *interp);

Tcl_Interp *vtkPVApplication::InitializeTcl(int argc, char *argv[])
{

  Tcl_Interp *interp = vtkKWApplication::InitializeTcl(argc,argv);
  
  //  if (Vtkparalleltcl_Init(interp) == TCL_ERROR) 
  //  {
   // cerr << "Init Parallel error\n";
   // }

  // Why is this here?  Doesn't the superclass initialize this?
  if (vtkKWApplication::GetWidgetVisibility())
    {
    Vtktkrenderwidget_Init(interp);
    }
   
  Vtkkwparaviewtcl_Init(interp);
 
  return interp;
}

//----------------------------------------------------------------------------
vtkPVApplication* vtkPVApplication::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVApplication");
  if(ret)
    {
    return (vtkPVApplication*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPVApplication;
}

int vtkPVApplicationCommand(ClientData cd, Tcl_Interp *interp,
			    int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVApplication::vtkPVApplication()
{
  this->CommandFunction = vtkPVApplicationCommand;
  this->SetApplicationName("ParaView");

  this->Controller = NULL;

  vtkOutputWindow::GetInstance()->PromptUserOn();


  // For some reason "GetObjectFromPointer" is returning vtkTemp0 instead
  /// of Application.  Lets force it.
  //if (this->TclName != NULL)
  //  {
  //  vtkErrorMacro("Expecting TclName to be NULL.");
  //  }
  //this->TclName = new char [strlen("Application")+1];
  //strcpy(this->TclName,"Application");
}


//----------------------------------------------------------------------------
void vtkPVApplication::RemoteScript(int id, char *format, ...)
{
  static char event[16000];
  
  va_list var_args;
  va_start(var_args, format);
  vsprintf(event, format, var_args);
  va_end(var_args);

  this->RemoteSimpleScript(id, event);
}
//----------------------------------------------------------------------------
void vtkPVApplication::RemoteSimpleScript(int remoteId, char *str)
{
  int length;

  if (this->Controller->GetLocalProcessId() == remoteId)
    {
    this->SimpleScript(str);
    return;
    }
  
  // send string to evaluate.
  length = strlen(str) + 1;
  if (length <= 1)
    {
    return;
    }

  //cerr << "---- RemoteScript, id = " << remoteId << ", str = " << str << endl;
  
  this->Controller->TriggerRMI(remoteId, str, VTK_PV_SLAVE_SCRIPT_RMI_TAG);
}

//----------------------------------------------------------------------------
void vtkPVApplication::BroadcastScript(char *format, ...)
{
  static char event[16000];
  
  va_list var_args;
  va_start(var_args, format);
  vsprintf(event, format, var_args);
  va_end(var_args);

  this->BroadcastSimpleScript(event);
}

//----------------------------------------------------------------------------
void vtkPVApplication::BroadcastSimpleScript(char *str)
{
  int id, num;
  
  num = this->Controller->GetNumberOfProcesses();

  for (id = 1; id < num; ++id)
    {
    this->RemoteSimpleScript(id, str);
    }
  
  // Do reverse order, because 0 will block.
  this->SimpleScript(str);
}



//----------------------------------------------------------------------------
int vtkPVApplication::AcceptLicense()
{
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVApplication::AcceptEvaluation()
{
  return 1;
}

//----------------------------------------------------------------------------
int VerifyKey(unsigned long key, const char *name, int id)
{
 return 1;
}

//----------------------------------------------------------------------------
int vtkPVApplication::PromptRegistration(char *name, char *IDS)
{
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVApplication::CheckRegistration()
{
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVApplication::Start(int argc, char*argv[])
{
  
  vtkPVWindow *ui = vtkPVWindow::New();
  this->Windows->AddItem(ui);

  this->CreateButtonPhotos();
  ui->Create(this,"");
  
  if (argc > 1 && argv[1])
    {
    // if a tcl script was passed in as an arg then load it
    if (!strcmp(argv[1] + strlen(argv[1]) - 4,".tcl"))
      {
      ui->LoadScript(argv[1]);
      }
    // otherwise try to load it as a volume
    else
      {
      if (strlen(argv[1]) > 1)
        {
        //ui->Open(argv[1]);
        }
      }
    }

  ui->Delete();
  this->vtkKWApplication::Start(argc,argv);
}

//----------------------------------------------------------------------------
void vtkPVApplication::DisplayAbout(vtkKWWindow *win)
{
  
  if (!this->AcceptLicense())
    {
    this->Exit();
    }
}

//----------------------------------------------------------------------------
void vtkPVApplication::Exit()
{
  int id, myId, num;
  
  // Send a break RMI to each of the slaves.
  num = this->Controller->GetNumberOfProcesses();
  myId = this->Controller->GetLocalProcessId();
  
  for (id = 0; id < num; ++id)
    {
    if (id != myId)
      {
      this->Controller->TriggerRMI(id, vtkMultiProcessController::BREAK_RMI_TAG);
      }
    }
  this->vtkKWApplication::Exit();
}


//----------------------------------------------------------------------------
void vtkPVApplication::SendDataBounds(vtkDataSet *data)
{
  float *bounds;
  
  if (this->Controller->GetLocalProcessId() == 0)
    {
    return;
    }
  bounds = data->GetBounds();
  this->Controller->Send(bounds, 6, 0, 1967);
}


//----------------------------------------------------------------------------
void vtkPVApplication::SendDataNumberOfCells(vtkDataSet *data)
{
  int num;
  
  if (this->Controller->GetLocalProcessId() == 0)
    {
    return;
    }
  num = data->GetNumberOfCells();
  this->Controller->Send(&num, 1, 0, 1968);
}

//----------------------------------------------------------------------------
void vtkPVApplication::SendDataNumberOfPoints(vtkDataSet *data)
{
  int num;
  
  if (this->Controller->GetLocalProcessId() == 0)
    {
    return;
    }
  num = data->GetNumberOfPoints();
  this->Controller->Send(&num, 1, 0, 1969);
}

//----------------------------------------------------------------------------
void vtkPVApplication::SendMapperColorRange(vtkPolyDataMapper *mapper)
{
  float range[2];
  vtkScalars *colors;

  if (this->Controller->GetLocalProcessId() == 0)
    {
    return;
    }
  colors = mapper->GetColors();
  if (colors == NULL)
    {
    range[0] = VTK_LARGE_FLOAT;
    range[1] = -VTK_LARGE_FLOAT;
    }
  else
    {
    colors->GetRange(range);
    }
  this->Controller->Send(range, 2, 0, 1969);
}


//----------------------------------------------------------------------------
void vtkPVApplication::CreateButtonPhotos()
{
  this->CreatePhoto("KWResetViewButton", KW_RESET_VIEW_BUTTON, 
              KW_RESET_VIEW_BUTTON_WIDTH, KW_RESET_VIEW_BUTTON_HEIGHT);
  this->CreatePhoto("KWTranslateViewButton", KW_TRANSLATE_VIEW_BUTTON, 
              KW_TRANSLATE_VIEW_BUTTON_WIDTH, KW_TRANSLATE_VIEW_BUTTON_HEIGHT);
  this->CreatePhoto("KWActiveTranslateViewButton", KW_ACTIVE_TRANSLATE_VIEW_BUTTON, 
              KW_ACTIVE_TRANSLATE_VIEW_BUTTON_WIDTH, KW_ACTIVE_TRANSLATE_VIEW_BUTTON_HEIGHT);

  this->CreatePhoto("KWFlyButton", KW_FLY_BUTTON, 
              KW_FLY_BUTTON_WIDTH, KW_FLY_BUTTON_HEIGHT);
  this->CreatePhoto("KWActiveFlyButton", KW_ACTIVE_FLY_BUTTON, 
              KW_ACTIVE_FLY_BUTTON_WIDTH, KW_ACTIVE_FLY_BUTTON_HEIGHT);
  this->CreatePhoto("KWRotateViewButton", KW_ROTATE_VIEW_BUTTON, 
              KW_ROTATE_VIEW_BUTTON_WIDTH, KW_ROTATE_VIEW_BUTTON_HEIGHT);
  this->CreatePhoto("KWActiveRotateViewButton", KW_ACTIVE_ROTATE_VIEW_BUTTON, 
              KW_ACTIVE_ROTATE_VIEW_BUTTON_WIDTH, KW_ACTIVE_ROTATE_VIEW_BUTTON_HEIGHT);
  this->CreatePhoto("KWPickCenterButton", KW_PICK_CENTER_BUTTON, 
              KW_PICK_CENTER_BUTTON_WIDTH, KW_PICK_CENTER_BUTTON_HEIGHT);
}

//----------------------------------------------------------------------------
void vtkPVApplication::CreatePhoto(char *name, unsigned char *data, 
                                    int width, int height)
{
  Tk_PhotoHandle photo;
  Tk_PhotoImageBlock block;

  this->Script("image create photo %s -height %d -width %d", 
               name, width, height);
  block.width = width;
  block.height = height;
  block.pixelSize = 3;
  block.pitch = block.width*block.pixelSize;
  block.offset[0] = 0;
  block.offset[1] = 1;
  block.offset[2] = 2;
  block.pixelPtr = data;

  photo = Tk_FindPhoto(this->GetMainInterp(), name);
  if (!photo)
    {
    vtkWarningMacro("error looking up color ramp image");
    return;
    }  
  Tk_PhotoPutBlock(photo, &block, 0, 0, block.width, block.height);

}


//============================================================================
// Make instances of sources.
//============================================================================

//----------------------------------------------------------------------------
vtkObject *vtkPVApplication::MakeTclObject(const char *className,
                                           const char *tclName)
{
  vtkObject *o;
  int error;

  this->BroadcastScript("%s %s", className, tclName);
  o = (vtkObject *)(vtkTclGetPointerFromObject(tclName,
                                  "vtkObject", this->GetMainInterp(), error));
  
  if (o == NULL)
    {
    vtkErrorMacro("Could not get object from pointer.");
    }
  
  return o;
}
