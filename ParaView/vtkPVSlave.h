/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVSlave.h
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
// .NAME vtkPVSlave - manage all the windows in an application
// .SECTION Description
// vtkPVSlave is the overall class that represents the entire 
// applicaiton. It is a fairly small class that is primarily responsible
// for managing the all the vtkKWWindows in the application. 


#ifndef __vtkPVSlave_h
#define __vtkPVSlave_h

#include "vtkKWObject.h"
#include "vtkMultiProcessController.h"
#include "tcl.h"
#include "tk.h"

class vtkKWWindowCollection;
class vtkKWWindow;
class vtkKWWidget;
class vtkKWEventNotifier;


#define VTK_PV_SLAVE_SCRIPT_RMI_TAG 1150
#define VTK_PV_SLAVE_SCRIPT_COMMAND_LENGTH_TAG 1100
#define VTK_PV_SLAVE_SCRIPT_COMMAND_TAG 1120
#define VTK_PV_SLAVE_SCRIPT_RESULT_LENGTH_TAG 1130
#define VTK_PV_SLAVE_SCRIPT_RESULT_TAG 1140


//BTX
// Adds an RMI to initialize the interpreter and start the RMI loop.
extern "C" void vtkPVSlaveStart(vtkMultiProcessController *controller);
//ETX


class VTK_EXPORT vtkPVSlave : public vtkKWObject
{
public:
  static vtkPVSlave* New();
  vtkTypeMacro(vtkPVSlave,vtkKWObject);
  
  // Description:
  // Keep the controller in this object.
  // (We no longer have a global controller.)
  vtkSetObjectMacro(Controller, vtkMultiProcessController);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  
  // Description:
  // Get the interpreter being used by this application
  Tcl_Interp *GetInterp() {return this->Interp;};
  void SetInterp(Tcl_Interp *interp);
  
  // Description:
  // IVars that indicate how many slaves there are, and which one are we.
  vtkSetMacro(NumberOfSlaves, int);
  vtkGetMacro(NumberOfSlaves, int);
  vtkSetMacro(SlaveId, int);
  vtkGetMacro(SlaveId, int);
  
//BTX
  // Description:
  // A convienience method to invoke some tcl script code and
  // perform arguement substitution.
  void Script(char *EventString, ...);
  void SimpleScript(char *EventString);
//ETX

protected:
  vtkPVSlave();
  ~vtkPVSlave();
  vtkPVSlave(const vtkPVSlave&) {};
  void operator=(const vtkPVSlave&) {};

  vtkKWWindowCollection *Windows;

  int NumberOfSlaves;
  int SlaveId;

  Tcl_Interp *Interp;
  vtkMultiProcessController *Controller;
  
};

#endif


