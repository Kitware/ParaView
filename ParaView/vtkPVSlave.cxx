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
  this->Controller = NULL;
}

vtkPVSlave::~vtkPVSlave()
{
  this->SetController(NULL);
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


void vtkPVSlave::Script(char *format, ...)
{
  static char event[16000];
  
  va_list var_args;
  va_start(var_args, format);
  vsprintf(event, format, var_args);
  va_end(var_args);

  this->SimpleScript(event);
}
void vtkPVSlave::SimpleScript(char *event)
{
//#define VTK_DEBUG_SCRIPT
#ifdef VTK_DEBUG_SCRIPT
  vtkOutputWindow::GetInstance()->DisplayText(event);
  vtkOutputWindow::GetInstance()->DisplayText("\n");
#endif
  
  //cerr << this->Controller->GetLocalProcessId() << ": interp (" << this->Interp << "): " << event << endl;  
  
  if (Tcl_Eval(this->Interp, event) != TCL_OK)
    {
    vtkGenericWarningMacro("In Script (" << this->Interp << "): " 
			   << event << "\nReceived Error: " << this->Interp->result);
    }
}






