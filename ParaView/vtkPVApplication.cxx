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

#include "vtkPVSlave.h"

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

//----------------------------------------------------------------------------
vtkPVApplication::vtkPVApplication()
{
  this->SetApplicationName("ParaView");
}

//----------------------------------------------------------------------------
void vtkPVApplication::RemoteScript(int remoteId, char *str, char *result, int resultMax)
{
  int length;
  vtkMultiProcessController *controller;

  controller = vtkMultiProcessController::RegisterAndGetGlobalController(this);

  // send string to evaluate.
  length = strlen(str) + 1;
  if (length <= 1)
    {
    return;
    }

  controller->TriggerRMI(remoteId, VTK_PV_SLAVE_SCRIPT_RMI_TAG);

  controller->Send(&length, 1, remoteId, VTK_PV_SLAVE_SCRIPT_COMMAND_LENGTH_TAG);
  controller->Send(str, length, remoteId, VTK_PV_SLAVE_SCRIPT_COMMAND_TAG);

  // Receive the result.
  controller->Receive(&length, 1, remoteId, VTK_PV_SLAVE_SCRIPT_RESULT_LENGTH_TAG);
  str = new char[length];
  controller->Receive(str, length, remoteId, VTK_PV_SLAVE_SCRIPT_RESULT_TAG);

  cerr << "Master: " << length << " " << str << endl;

  if (result && resultMax > 0)
    { 
    strncpy(result, str, resultMax-1);
    }
  
  controller->UnRegister(this);
  delete [] str;
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

#ifdef _WIN32
//----------------------------------------------------------------------------
void ReadAValue(HKEY hKey,char *val,char *key, char *adefault)
{
}
#endif

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
  
  if (argc > 1)
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
  vtkMultiProcessController *controller;
  
  // Send a break RMI to each of the slaves.
  controller = vtkMultiProcessController::RegisterAndGetGlobalController(this);
  num = controller->GetNumberOfProcesses();
  myId = controller->GetLocalProcessId();
  
  for (id = 0; id < num; ++id)
    {
    if (id != myId)
      {
      controller->TriggerRMI(id, VTK_BREAK_RMI_TAG);
      }
    }
  
  this->vtkKWApplication::Exit();
}
