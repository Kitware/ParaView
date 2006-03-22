/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPickBoxWidgetProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMPickBoxWidgetProxy
// .SECTION Description


#ifndef __vtkSMPickBoxWidgetProxy_h
#define __vtkSMPickBoxWidgetProxy_h

#include "vtkSMBoxWidgetProxy.h"

class VTK_EXPORT vtkSMPickBoxWidgetProxy : public vtkSMBoxWidgetProxy
{
public:
  static vtkSMPickBoxWidgetProxy* New();
  vtkTypeRevisionMacro(vtkSMPickBoxWidgetProxy, vtkSMBoxWidgetProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Called to push the values onto the VTK object.
  virtual void UpdateVTKObjects();

// ATTRIBUTE EDITOR 
  vtkSetMacro(MouseControlToggle,int);
  vtkGetMacro(MouseControlToggle,int);


protected:
  //BTX
  vtkSMPickBoxWidgetProxy();
  ~vtkSMPickBoxWidgetProxy();
  
// ATTRIBUTE EDITOR
  int MouseControlToggle;

private:
  vtkSMPickBoxWidgetProxy(const vtkSMPickBoxWidgetProxy&); // Not implemented
  void operator=(const vtkSMPickBoxWidgetProxy&); // Not implemented
  //ETX
};

#endif
