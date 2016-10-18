/*=========================================================================

  Program:   ParaView
  Module:    vtkSIStringVectorProperty.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSIStringVectorProperty
 *
 * ServerImplementation Property to deal with String array as method arguments.
*/

#ifndef vtkSIStringVectorProperty_h
#define vtkSIStringVectorProperty_h

#include "vtkPVServerImplementationCoreModule.h" //needed for exports
#include "vtkSIVectorProperty.h"

class VTKPVSERVERIMPLEMENTATIONCORE_EXPORT vtkSIStringVectorProperty : public vtkSIVectorProperty
{
public:
  static vtkSIStringVectorProperty* New();
  vtkTypeMacro(vtkSIStringVectorProperty, vtkSIVectorProperty);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkSIStringVectorProperty();
  ~vtkSIStringVectorProperty();

  enum ElementTypes
  {
    INT,
    DOUBLE,
    STRING
  };

  /**
   * Push a new state to the underneath implementation
   */
  virtual bool Push(vtkSMMessage*, int);

  /**
   * Pull the current state of the underneath implementation
   */
  virtual bool Pull(vtkSMMessage*);

  /**
   * Parse the xml for the property.
   */
  virtual bool ReadXMLAttributes(vtkSIProxy* proxyhelper, vtkPVXMLElement* element);

private:
  vtkSIStringVectorProperty(const vtkSIStringVectorProperty&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSIStringVectorProperty&) VTK_DELETE_FUNCTION;

  class vtkVectorOfStrings;
  class vtkVectorOfInts;

  bool Push(const vtkVectorOfStrings& values);
  vtkVectorOfInts* ElementTypes;
};

#endif
