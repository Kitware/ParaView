/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVSlave.cxx
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
#include "vtkPVSlave.h"
#include "vtkKWObject.h"
#include "vtkTclUtil.h"
#include "vtkObjectFactory.h"
#include "vtkOutputWindow.h"
#include "vtkMultiProcessController.h"


//extern "C" int Vtktcl_Init(Tcl_Interp *interp);
extern "C" int Vtkcommontcl_Init(Tcl_Interp *interp);
extern "C" int Vtkcontribtcl_Init(Tcl_Interp *interp);
extern "C" int Vtkgraphicstcl_Init(Tcl_Interp *interp);
extern "C" int Vtkimagingtcl_Init(Tcl_Interp *interp);


// For setting up the tcl interpreter.
void Slave_Init(int myId, int numSlaves)
{
  Tcl_Interp *interp = Tcl_CreateInterp();

  
  cerr << myId << ",  of " << numSlaves << endl;
  
  
  //if (Tcl_Init(interp) == TCL_ERROR) 
  //  {
  //  cerr << "Init Tcl error\n";
  //  }

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
  
  if (Tcl_GlobalEval(interp, "load vtkKWWidgetsTcl") != TCL_OK)
    {
    cerr << "Error returned from tcl script.\n" << interp->result << endl;
    } 
  if (Tcl_GlobalEval(interp, "load vtkKWParaViewTcl") != TCL_OK)
    {
    cerr << "Error returned from tcl script.\n" << interp->result << endl;
    }
  
  // Set up the slave object.
  if (Tcl_GlobalEval(interp, "vtkPVSlave Slave") != TCL_OK)
    {
    cerr << "Error returned from tcl script.\n" << interp->result << endl;
    }
  int    error;
  vtkPVSlave *slave = (vtkPVSlave *)(vtkTclGetPointerFromObject("Slave","vtkPVSlave",interp,error));
  if (slave == NULL)
    {
    vtkGenericWarningMacro("Could not get slave pointer.");
    return;
    }
  
  slave->SetInterp(interp);
  slave->SetNumberOfSlaves(numSlaves);
  slave->SetSlaveId(myId);
  slave->Start();
}










//------------------------------------------------------------------------------
vtkPVSlave* vtkPVSlave::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVSlave");
  if(ret)
    {
    return (vtkPVSlave*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPVSlave;
}


int vtkPVSlaveCommand(ClientData cd, Tcl_Interp *interp,
			    int argc, char *argv[]);

vtkPVSlave::vtkPVSlave()
{
  this->Interp = NULL;
  
  this->SlaveId = -1;
  this->NumberOfSlaves = 0;
}

vtkPVSlave::~vtkPVSlave()
{
}


void vtkPVSlave::SetInterp(Tcl_Interp *interp)
{
  if (this->Interp == interp)
    {
    return;
    }
  this->Interp = interp;
  this->Modified();
}


char *vtkPVSlave::Script(char *format, ...)
{
  static char event[16000];
  
  va_list var_args;
  va_start(var_args, format);
  vsprintf(event, format, var_args);
  va_end(var_args);

  return this->SimpleScript(event);
}
char * vtkPVSlave::SimpleScript(char *event)
{
//#define VTK_DEBUG_SCRIPT
#ifdef VTK_DEBUG_SCRIPT
    vtkOutputWindow::GetInstance()->DisplayText(event);
    vtkOutputWindow::GetInstance()->DisplayText("\n");
#endif
  
  if (Tcl_Eval(this->Interp, event) != TCL_OK)
    {
    vtkGenericWarningMacro("Error returned from tcl script.\n" <<
			   this->Interp->result << endl);
    }
  return this->Interp->result;
}


void vtkPVSlave::SlaveScript(int otherId)
{
  int length;
  char *str, *result;
  vtkMultiProcessController *controller;

  controller = vtkMultiProcessController::RegisterAndGetGlobalController(this);

  // Receive string to evaluate.
  controller->Receive(&length, 1, otherId, VTK_PV_SLAVE_SCRIPT_COMMAND_LENGTH_TAG);
  if (length <= 0)
    {
    cerr << "ERROR: NULL script\n";
    }
  str = new char[length];
  controller->Receive(str, length, otherId, VTK_PV_SLAVE_SCRIPT_COMMAND_TAG);

  // Evalute string ...
  result = this->SimpleScript(str);

  if (result == NULL)
    {
    vtkErrorMacro("ERROR: NULL result");
    }

  // Send result back.
  length = strlen(result) + 1;
  controller->Send(&length, 1, otherId, VTK_PV_SLAVE_SCRIPT_RESULT_LENGTH_TAG);
  controller->Send(result, length, otherId, VTK_PV_SLAVE_SCRIPT_RESULT_TAG);

  delete [] str;
  controller->UnRegister(this);  
}


void vtkPVSlaveScript(void *arg, int remoteProcessId)
{
  vtkPVSlave *self = (vtkPVSlave *)(arg);

  self->SlaveScript(remoteProcessId);
}



void vtkPVSlave::Start()
{ 
  vtkMultiProcessController *controller = vtkMultiProcessController::RegisterAndGetGlobalController(this);
  
  controller->AddRMI(vtkPVSlaveScript, (void *)(this), VTK_PV_SLAVE_SCRIPT_RMI_TAG);
  
  controller->ProcessRMIs();
}





