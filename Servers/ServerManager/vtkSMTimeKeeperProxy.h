/*=========================================================================

  Program:   ParaView
  Module:    vtkSMTimeKeeperProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMTimeKeeperProxy - a time keeper is used to keep track of the
// pipeline time.
// .SECTION Description
// In ServerManager, the pipeline time is should be set on views. TimeKeeper is
// a proxy that can be used to keep views linked together so that they show the
// same pipeline time. In that case, to change the view time, one must simply
// change the "Time" on the time keeper.
// This proxy has no VTK objects that it creates on the server.

#ifndef __vtkSMTimeKeeperProxy_h
#define __vtkSMTimeKeeperProxy_h

#include "vtkSMProxy.h"

class vtkSMViewProxy;
class vtkCollection;

class VTK_EXPORT vtkSMTimeKeeperProxy : public vtkSMProxy
{
public:
  static vtkSMTimeKeeperProxy* New();
  vtkTypeRevisionMacro(vtkSMTimeKeeperProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the pipeline time.
  void SetTime(double time);
  vtkGetMacro(Time, double);

  // Description:
  // Add/Remove view proxy linked to this time keeper.
  void AddView(vtkSMViewProxy*);
  void RemoveView(vtkSMViewProxy*);
  void RemoveAllViews();

//BTX
protected:
  vtkSMTimeKeeperProxy();
  ~vtkSMTimeKeeperProxy();

  double Time;
  vtkCollection* Views;
private:
  vtkSMTimeKeeperProxy(const vtkSMTimeKeeperProxy&); // Not implemented
  void operator=(const vtkSMTimeKeeperProxy&); // Not implemented
//ETX
};

#endif

