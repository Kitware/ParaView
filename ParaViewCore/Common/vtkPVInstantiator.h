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
// .NAME vtkPVInstantiator - similar to vtkInstantiator except that it uses
// ParaView's client-server streams to create new classes.
// .SECTION Description
// vtkPVInstantiator is similar to vtkInstantiator except that it uses
// ParaView's client-server streams to create new classes. Thus we don't have to
// do any additional initialization as needed for vtkInstantiator to work.

#ifndef __vtkPVInstantiator_h
#define __vtkPVInstantiator_h

#include "vtkObject.h"
#include "vtkPVCommonModule.h" // needed for export macro

class VTKPVCOMMON_EXPORT vtkPVInstantiator : public vtkObject
{
public:
  static vtkPVInstantiator* New();
  vtkTypeMacro(vtkPVInstantiator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create an instance of the class whose name is given.  If creation
  // fails, a NULL pointer is returned.
  // This uses vtkClientServerInterpreter::NewInstance() to create the class.
  static vtkObject* CreateInstance(const char* className);

//BTX
protected:
  vtkPVInstantiator();
  ~vtkPVInstantiator();

private:
  vtkPVInstantiator(const vtkPVInstantiator&); // Not implemented
  void operator=(const vtkPVInstantiator&); // Not implemented
//ETX
};

#endif
