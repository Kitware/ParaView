/*=========================================================================

  Program:   ParaView
  Module:    vtkClientServerStreamInstantiator.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkClientServerStreamInstantiator
 * @brief creates instances of vtkObjectBase subclasses given the name.
 *
 * vtkClientServerStreamInstantiator uses vtkClientServerStream and global
 * vtkClientServerInterpreter to create new instances of vtkObjectBase subclasses
 * given the string name.
 *
 */

#ifndef vtkClientServerStreamInstantiator_h
#define vtkClientServerStreamInstantiator_h

#include "vtkObject.h"
#include "vtkRemotingClientServerStreamModule.h" // Top-level vtkClientServer header.

class VTKREMOTINGCLIENTSERVERSTREAM_EXPORT vtkClientServerStreamInstantiator : public vtkObject
{
public:
  static vtkClientServerStreamInstantiator* New();
  vtkTypeMacro(vtkClientServerStreamInstantiator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Create an instance of the class whose name is given.  If creation
   * fails, a nullptr is returned.
   * This uses `vtkClientServerInterpreter::NewInstance()` to create the class.
   */
  VTK_NEWINSTANCE
  static vtkObjectBase* CreateInstance(const char* className);

protected:
  vtkClientServerStreamInstantiator();
  ~vtkClientServerStreamInstantiator() override;

private:
  vtkClientServerStreamInstantiator(const vtkClientServerStreamInstantiator&) = delete;
  void operator=(const vtkClientServerStreamInstantiator&) = delete;
};

#endif
