/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ParaView.cxx
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
#include "vtkObject.h"
#include "vtkMultiProcessController.h"
#include "vtkPVApplication.h"
#include "vtkTclUtil.h"
#include "kwinit.h"

extern "C" int Vtktcl_Init(Tcl_Interp *interp);
extern "C" int Vtkcommontcl_Init(Tcl_Interp *interp);
extern "C" int Vtkcontribtcl_Init(Tcl_Interp *interp);
extern "C" int Vtkgraphicstcl_Init(Tcl_Interp *interp);
extern "C" int Vtkimagingtcl_Init(Tcl_Interp *interp);
extern "C" int Vtkpatentedtcl_Init(Tcl_Interp *interp);

extern "C" int Vtkkwwidgetstcl_Init(Tcl_Interp *interp);
extern "C" int Vtkkwparaviewtcl_Init(Tcl_Interp *interp);


// external global variable.
vtkMultiProcessController *VTK_PV_UI_CONTROLLER = NULL;


struct vtkPVArgs
{
  int argc;
  char **argv;
};



void vtkPVSlaveScript(void *localArg, void *remoteArg, int remoteArgLength,
		      int remoteProcessId)
{
  vtkPVApplication *self = (vtkPVApplication *)(localArg);

  self->SimpleScript((char*)remoteArg);
}




// The applications Initialize Tcl also initializes Tk and needs a DISPLAY env variable set.
// This is a temporary solution.
Tcl_Interp *vtkPVInitializeTcl()
{
  Tcl_Interp *interp;
  
  interp = Tcl_CreateInterp();
  // vtkKWApplication depends on this variable being set.
  Et_Interp = interp;
  
  // initialize VTK
  Vtktcl_Init(interp);
  
  if (Tcl_Init(interp) == TCL_ERROR) 
    {
    cerr << "Init Tcl error\n";
    }
#ifndef WIN32
  if (Vtkcommontcl_Init(interp) == TCL_ERROR) 
    {
    cerr << "Init Common error\n";
    }
    
  if (Vtkgraphicstcl_Init(interp) == TCL_ERROR) 
    {
    cerr << "Init Graphics error\n";
    }
  if (Vtkimagingtcl_Init(interp) == TCL_ERROR) 
    {
    cerr << "Init Imaging error\n";
    }
  if (Vtkcontribtcl_Init(interp) == TCL_ERROR) 
    {
    cerr << "Init Contrib error\n";
    }
  if (Vtkpatentedtcl_Init(interp) == TCL_ERROR) 
    {
    cerr << "Init Patented error\n";
    }
#endif

  if (Vtkkwwidgetstcl_Init(interp) == TCL_ERROR) 
    {
    cerr << "Init KWWidgets error\n";
    }
  if (Vtkkwparaviewtcl_Init(interp) == TCL_ERROR) 
    {
    cerr << "Init KWParaView error\n";
    }

  return interp;
}




// Each process starts with this method.  One process is designated as "master" 
// and starts the application.  The other processes are slaves to the application.
void Process_Init(vtkMultiProcessController *controller, void *arg )
{
  vtkPVArgs *pvArgs = (vtkPVArgs *)arg;
  int myId, numProcs;
  
  myId = controller->GetLocalProcessId();
  numProcs = controller->GetNumberOfProcesses();
  
  if (myId ==  0)
    { // The last process is for UI.
    // We need to pass the local controller to the UI process.
    Tcl_Interp *interp = vtkPVApplication::InitializeTcl(pvArgs->argc,pvArgs->argv);
    
    // To bypass vtkKWApplicaion assigning vtkKWApplicationCommand
    // to the tcl command, create the application from tcl.
    if (Tcl_Eval(interp, "vtkPVApplication Application") != TCL_OK)
      {
      cerr << "Error returned from tcl script.\n" << interp->result << endl;
      }
    int    error;
    vtkPVApplication *app = (vtkPVApplication *)(
      vtkTclGetPointerFromObject("Application","vtkPVApplication",
				 interp,error));
    if (app == NULL)
      {
      vtkGenericWarningMacro("Could not get application pointer.");
      return;
      }  
    
    app->SetController(controller);
    app->Script("wm withdraw .");
    app->Start(pvArgs->argc,pvArgs->argv);
    app->Delete();
    }
  else
    {
    // The slaves try to connect.  In the future, we may not want to initialize Tk.
    //putenv("DISPLAY=:0.0");
    //putenv("DISPLAY=www.kitware.com:2.0");
    
    vtkKWApplication::SetWidgetVisibility(0);
    //Tcl_Interp *interp = vtkPVApplication::InitializeTcl(pvArgs->argc,pvArgs->argv);
    Tcl_Interp *interp = vtkPVInitializeTcl();
    
    // We should use the application tcl name in the future.
    // All object in the satellite processes must be created through tcl.
    // (To assign the correct name).
    if (Tcl_Eval(interp, "vtkPVApplication Application") != TCL_OK)
      {
      cerr << "Error returned from tcl script.\n" << interp->result << endl;
      }
    int    error;
    vtkPVApplication *app = (vtkPVApplication *)(
      vtkTclGetPointerFromObject("Application","vtkPVApplication",
				 interp,error));
    if (app == NULL)
      {
      vtkGenericWarningMacro("Could not get application pointer.");
      return;
      }  
    app->SetController(controller);
    controller->AddRMI(vtkPVSlaveScript, (void *)(app), VTK_PV_SLAVE_SCRIPT_RMI_TAG);
    controller->ProcessRMIs();
    }
}

#ifdef _WIN32
#include <windows.h>



int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nShowCmd)
{
  int argc;
  char *argv[5];

  argv[0] = new char [strlen(lpCmdLine)+1];
  argv[1] = new char [strlen(lpCmdLine)+1];
  argv[2] = new char [strlen(lpCmdLine)+1];
  argv[3] = new char [strlen(lpCmdLine)+1];
  argv[4] = new char [strlen(lpCmdLine)+1];

  unsigned int i;
  // parse a few of the command line arguments
  // a space delimites an argument except when it is inside a quote
  argc = 1;
  int pos = 0;
  for (i = 0; i < strlen(lpCmdLine); i++)
    {
    while (lpCmdLine[i] == ' ' && i < strlen(lpCmdLine))
      {
      i++;
      }
    if (lpCmdLine[i] == '\"')
      {
      i++;
      while (lpCmdLine[i] != '\"' && i < strlen(lpCmdLine))
        {
        argv[argc][pos] = lpCmdLine[i];
        i++;
        pos++;
        }
      argv[argc][pos] = '\0';
      argc++;
      pos = 0;
      }
    else
      {
      while (lpCmdLine[i] != ' ' && i < strlen(lpCmdLine))
        {
        argv[argc][pos] = lpCmdLine[i];
        i++;
        pos++;
        }
      argv[argc][pos] = '\0';
      argc++;
      pos = 0;
      }
    }

  vtkMultiProcessController *controller = vtkMultiProcessController::New();
  controller->Initialize(argc, argv);
  
  Tcl_Interp *interp = vtkPVApplication::InitializeTcl(argc,argv);
  vtkPVApplication *app = vtkPVApplication::New();
  app->SetController(controller);
  app->Script("wm withdraw .");
    
  app->Start(argc,argv);
  app->Delete();
  
  controller->Delete();
  
  delete [] argv[0];
  delete [] argv[1];
  delete [] argv[2];
  delete [] argv[3];
  delete [] argv[4];
  return 0;
}
#else
int main(int argc, char *argv[])
{
  // New processes need these args to initialize.
  vtkPVArgs pvArgs;
  pvArgs.argc = argc;
  pvArgs.argv = argv;

  vtkMultiProcessController *controller = vtkMultiProcessController::New();  
  controller->Initialize(argc, argv);
  //controller->SetNumberOfProcesses(2);

  controller->SetSingleMethod(Process_Init, (void *)(&pvArgs));
  controller->SingleMethodExecute();

  //Process_Init((void *)(&pvArgs));
  
  controller->Delete();
  
  return 0;
}
#endif



























