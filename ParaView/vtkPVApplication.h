/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVApplication.h
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
// .NAME vtkPVApplication
// .SECTION Description
// A subclass of vtkKWApplication specific to this application.

#ifndef __vtkPVApplication_h
#define __vtkPVApplication_h

#include "vtkKWApplication.h"
#include "vtkMultiProcessController.h"

class vtkPVSource;


#define VTK_PV_SLAVE_SCRIPT_RMI_TAG 1150
#define VTK_PV_SLAVE_SCRIPT_COMMAND_LENGTH_TAG 1100
#define VTK_PV_SLAVE_SCRIPT_COMMAND_TAG 1120
#define VTK_PV_SLAVE_SCRIPT_RESULT_LENGTH_TAG 1130
#define VTK_PV_SLAVE_SCRIPT_RESULT_TAG 1140


class VTK_EXPORT vtkPVApplication : public vtkKWApplication
{
public:
  static vtkPVApplication* New();
  vtkTypeMacro(vtkPVApplication,vtkKWApplication);
  
  // Description:
  // Start running the main application.
  virtual void Start(int argc, char *argv[]);

  
//BTX
  // Description:
  // Script which is executed in the remot processes.
  // If a result string is passed in, the results are place in it. 
  void RemoteScript(int remoteId, char *EventString, ...);

  // Description:
  // Can only be called by process 0.  It executes a script on every other process.
  void BroadcastScript(char *EventString, ...);
//ETX
  void RemoteSimpleScript(int remoteId, char *str);
  void BroadcastSimpleScript(char *str);

  
  
  // Description:
  // We need to keep the controller in a prominent spot because there is no more 
  // "RegisterAndGetGlobalController" method.
  vtkSetObjectMacro(Controller, vtkMultiProcessController);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  
  // Description:
  // Display the on-line help and about dialog for this application.
  virtual void DisplayAbout(vtkKWWindow *);

  // Description:
  // Make sure the user accepts the license before running.
  int AcceptLicense();
  int AcceptEvaluation();

  // Description:
  // We need to kill the slave processes
  void Exit();
  
  // Description:
  // class static method to initialize Tcl/Tk
  static Tcl_Interp *InitializeTcl(int argc, char *argv[]);  

  // This constructs a vtk object (type specified by class name)
  // and uses the tclName for the tcl instance command.
  // The user must cast to the correct type, and is responsible
  // for deleting the object.
  vtkObject *MakeTclObject(const char *className,
                           const char *tclName);

  // Description:
  // When ParaView needs to query data on other procs, it needs a way to
  // get the information back (only VTK object on satellite procs).
  // These methods send the requested data to proc 0 with a tag of 1966.
  // Note:  Process 0 returns without sending.
  void SendDataScalarRange(vtkDataSet *data);
  
protected:
  vtkPVApplication();
  ~vtkPVApplication() {};
  vtkPVApplication(const vtkPVApplication&) {};
  void operator=(const vtkPVApplication&) {};
  
  int CheckRegistration();
  int PromptRegistration(char *,char *);
  
  vtkMultiProcessController *Controller;
};

#endif


