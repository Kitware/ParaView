/*=========================================================================

  Program:   ParaView
  Module:    vtkSMLineWidgetProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMLineWidgetProxy
// .SECTION Description

#ifndef __vtkSMLineWidgetProxy_h
#define __vtkSMLineWidgetProxy_h

#include "vtkSM3DWidgetProxy.h"

class VTK_EXPORT vtkSMLineWidgetProxy : public vtkSM3DWidgetProxy
{
public:
  static vtkSMLineWidgetProxy* New();
  vtkTypeRevisionMacro(vtkSMLineWidgetProxy, vtkSM3DWidgetProxy);
  void PrintSelf(ostream &os,vtkIndent indent);

  vtkSetVector3Macro(Point1,double);
  vtkSetVector3Macro(Point2,double);
  vtkGetVector3Macro(Point1,double);
  vtkGetVector3Macro(Point2,double);

//  virtual void SaveInBatchScript(ofstream *file);
  virtual void UpdateVTKObjects();
protected:
//BTX
  vtkSMLineWidgetProxy();
  ~vtkSMLineWidgetProxy();

  // Description:
  // Overloaded to update the property values before saving state
  virtual void SaveState(const char* name, ostream* file, vtkIndent indent);
  
  // Description:
  // Execute event of the 3D Widget.
  virtual void ExecuteEvent(vtkObject*, unsigned long, void*);
 
  virtual void CreateVTKObjects(int numObjects);
  
  double Point1[3];
  double Point2[3];
private:
  vtkSMLineWidgetProxy(const vtkSMLineWidgetProxy&);// Not implemented
  void operator=(const vtkSMLineWidgetProxy&); // Not implemented
//ETX
};  

#endif
