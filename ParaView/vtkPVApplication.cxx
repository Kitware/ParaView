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


extern "C" int Vtktkrenderwidget_Init(Tcl_Interp *interp);
extern "C" int Vtkkwparaviewtcl_Init(Tcl_Interp *interp);
extern "C" int Vtkparalleltcl_Init(Tcl_Interp *interp);

Tcl_Interp *vtkPVApplication::InitializeTcl(int argc, char *argv[])
{
  Tcl_Interp *interp = vtkKWApplication::InitializeTcl(argc,argv);
  
  if (Vtkparalleltcl_Init(interp) == TCL_ERROR) 
    {
    cerr << "Init Parallel error\n";
    }

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
  
  
  this->SimpleScript(str);
  
  num = this->Controller->GetNumberOfProcesses();
  for (id = 1; id < num; ++id)
    {
    this->RemoteSimpleScript(id, str);
    }
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
void vtkPVApplication::SendDataScalarRange(vtkDataSet *data)
{
  float range[2];
  
  if (this->Controller->GetLocalProcessId() == 0)
    {
    return;
    }
  data->GetScalarRange(range);
  if (data->GetNumberOfPoints() == 0 && data->GetNumberOfCells() == 0)
    {
    range[0] = VTK_LARGE_FLOAT;
    range[1] = -VTK_LARGE_FLOAT;
    }
  this->Controller->Send(range, 2, 0, 1966);
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
