/*=========================================================================

  Program:   ParaView
  Module:    vtkSMAxesProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMAxesProxy - proxy for the Axes actor.
// .SECTION Description
// vtkSMAxesProxy is the Display for the Axes actor. This is used 
// as the center axes in ParaView. Proxifying it makes the axes
// accessible in batch mode. The only reason why Axes is even a 
// separate display proxy instead of the the generic Axes source proxy,
// and the vtkSMSimpleDisplayProxy subclass is because we don't want the 
// additional overhead of update suppressor etc for the Axes proxy.
// .SECTION See Also
// vtkSMDisplayProxy 

#ifndef __vtkSMAxesProxy_h
#define __vtkSMAxesProxy_h

#include "vtkSMDisplayProxy.h"
class vtkPVWindow;

class VTK_EXPORT vtkSMAxesProxy : public vtkSMDisplayProxy
{
public:
  static vtkSMAxesProxy* New();
  vtkTypeRevisionMacro(vtkSMAxesProxy, vtkSMDisplayProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Save the proxy in batch script.
  virtual void SaveInBatchScript(ofstream* file);  

  // Description:
  // Called when the display is added/removed to/from a RenderModule.
  virtual void AddToRenderModule(vtkSMRenderModuleProxy*);
  virtual void RemoveFromRenderModule(vtkSMRenderModuleProxy*);

//BTX
protected:
  vtkSMAxesProxy();
  ~vtkSMAxesProxy();

  // Description:
  // Create all the VTK objects.
  virtual void CreateVTKObjects(int numObjects);
private:
  vtkSMAxesProxy(const vtkSMAxesProxy&); // Not implemented
  void operator=(const vtkSMAxesProxy&); // Not implemented
//ETX
};


#endif
