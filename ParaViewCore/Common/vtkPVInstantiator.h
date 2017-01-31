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
/**
 * @class   vtkPVInstantiator
 * @brief   similar to vtkInstantiator except that it uses
 * ParaView's client-server streams to create new classes.
 *
 * vtkPVInstantiator is similar to vtkInstantiator except that it uses
 * ParaView's client-server streams to create new classes. Thus we don't have to
 * do any additional initialization as needed for vtkInstantiator to work.
*/

#ifndef vtkPVInstantiator_h
#define vtkPVInstantiator_h

#include "vtkObject.h"
#include "vtkPVCommonModule.h" // needed for export macro

class VTKPVCOMMON_EXPORT vtkPVInstantiator : public vtkObject
{
public:
  static vtkPVInstantiator* New();
  vtkTypeMacro(vtkPVInstantiator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Create an instance of the class whose name is given.  If creation
   * fails, a NULL pointer is returned.
   * This uses vtkClientServerInterpreter::NewInstance() to create the class.
   */
  VTK_NEWINSTANCE
  static vtkObject* CreateInstance(const char* className);

protected:
  vtkPVInstantiator();
  ~vtkPVInstantiator();

private:
  vtkPVInstantiator(const vtkPVInstantiator&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVInstantiator&) VTK_DELETE_FUNCTION;
};

#endif
