/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMCatalystSession
// .SECTION Description
//

#ifndef __vtkSMCatalystSession_h
#define __vtkSMCatalystSession_h

#include "vtkSMSession.h"
class vtkSMProxy;

class VTK_EXPORT vtkSMCatalystSession : public vtkSMSession
{
public:
  static vtkSMCatalystSession* New();
  vtkTypeMacro(vtkSMCatalystSession, vtkSMSession);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  void SetVisualizationSession(vtkSMSession*);
  vtkGetObjectMacro(VisualizationSession, vtkSMSession);

  // Description:
  // Called to do any initializations after a successful session has been
  // established.
  virtual void Initialize();

  // Description:
  // Set the port number. This is the port on which the root data-server node
  // will open a server-socket to accept connections from VTK InSitu Library.
  vtkSetMacro(InsituPort, int);
  vtkGetMacro(InsituPort, int);

//BTX
protected:
  vtkSMCatalystSession();
  ~vtkSMCatalystSession();
  int InsituPort;
  vtkSMSession* VisualizationSession;
  vtkSmartPointer<vtkSMProxy> LiveInsituLink;

private:
  vtkSMCatalystSession(const vtkSMCatalystSession&); // Not implemented
  void operator=(const vtkSMCatalystSession&); // Not implemented
//ETX
};

#endif
