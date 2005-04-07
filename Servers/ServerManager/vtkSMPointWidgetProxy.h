/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPointWidgetProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMPointWidgetProxy
// .SECTION Description

#ifndef __vtkSMPointWidgetProxy_h
#define __vtkSMPointWidgetProxy_h

#include "vtkSM3DWidgetProxy.h"

class VTK_EXPORT vtkSMPointWidgetProxy : public vtkSM3DWidgetProxy
{
public:
  static vtkSMPointWidgetProxy* New();
  vtkTypeRevisionMacro(vtkSMPointWidgetProxy, vtkSM3DWidgetProxy);
  void PrintSelf(ostream &os,vtkIndent indent);

  vtkSetVector3Macro(Position,double);
  vtkGetVector3Macro(Position,double);
 
  virtual void SaveInBatchScript(ofstream *file);

  virtual void UpdateVTKObjects();
protected:
//BTX
  vtkSMPointWidgetProxy();
  ~vtkSMPointWidgetProxy();

  // Description:
  // Overloaded to update the property values before saving state
  virtual void SaveState(const char* name, ostream* file, vtkIndent indent);
  
  // Description:
  // Execute event of the 3D Widget.
  virtual void ExecuteEvent(vtkObject*, unsigned long, void*);
  virtual void CreateVTKObjects(int numObjects);

  double Position[3];
  
private:
  vtkSMPointWidgetProxy(const vtkSMPointWidgetProxy&);// Not implemented
  void operator=(const vtkSMPointWidgetProxy&); // Not implemented
//ETX
};

#endif
