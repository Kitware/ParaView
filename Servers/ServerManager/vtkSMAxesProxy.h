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
// vtkSMAxesProxy is the Displayer for the Axes actor. This is used 
// as the center axes in ParaView. Proxifying it makes the axes
// accessible in batch mode.
// .SECTION See Also
// vtkSMDisplayerProxy vtkSMProxy

#ifndef __vtkSMAxesProxy_h
#define __vtkSMAxesProxy_h

#include "vtkSMDisplayerProxy.h"
class vtkPVWindow;

class VTK_EXPORT vtkSMAxesProxy : public vtkSMDisplayerProxy
{
public:
  static vtkSMAxesProxy* New();
  vtkTypeRevisionMacro(vtkSMAxesProxy, vtkSMDisplayerProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Save the proxy in batch script.
  virtual void SaveInBatchScript(ofstream* file);  
//BTX
protected:
  vtkSMAxesProxy();
  ~vtkSMAxesProxy();

  friend class vtkPVWindow;
  // Description:
  // Create all the VTK objects.
  virtual void CreateVTKObjects(int numObjects);
private:
  vtkSMAxesProxy(const vtkSMAxesProxy&); // Not implemented
  void operator=(const vtkSMAxesProxy&); // Not implemented
//ETX
};


#endif
