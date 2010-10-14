/*=========================================================================

  Program:   ParaView
  Module:    vtkSMContextNamedOptionsProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMContextNamedOptionsProxy
// .SECTION Description
//

#ifndef __vtkSMContextNamedOptionsProxy_h
#define __vtkSMContextNamedOptionsProxy_h

#include "vtkSMProxy.h"

class vtkChart;
class vtkTable;

class VTK_EXPORT vtkSMContextNamedOptionsProxy : public vtkSMProxy
{
public:
  static vtkSMContextNamedOptionsProxy* New();
  vtkTypeMacro(vtkSMContextNamedOptionsProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkSMContextNamedOptionsProxy();
  ~vtkSMContextNamedOptionsProxy();

  // Description:
  // Called to update the property information on the property. It is assured
  // that the property passed in as an argument is a self property. Both the
  // overloads of UpdatePropertyInformation() call this method, so subclass can
  // override this method to perform special tasks.
  virtual void UpdatePropertyInformationInternal(vtkSMProperty*);

private:
  vtkSMContextNamedOptionsProxy(const vtkSMContextNamedOptionsProxy&); // Not implemented
  void operator=(const vtkSMContextNamedOptionsProxy&); // Not implemented
//ETX
};

#endif
