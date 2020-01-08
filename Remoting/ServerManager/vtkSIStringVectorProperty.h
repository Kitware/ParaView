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
 * It also takes care of string encoding on server side.
*/

#ifndef vtkSIStringVectorProperty_h
#define vtkSIStringVectorProperty_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSIVectorProperty.h"

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSIStringVectorProperty : public vtkSIVectorProperty
{
public:
  static vtkSIStringVectorProperty* New();
  vtkTypeMacro(vtkSIStringVectorProperty, vtkSIVectorProperty);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkSIStringVectorProperty();
  ~vtkSIStringVectorProperty() override;

  enum ElementTypes
  {
    INT,
    DOUBLE,
    STRING
  };

  /**
   * Push a new state to the underneath implementation
   */
  bool Push(vtkSMMessage*, int) override;

  /**
   * Pull the current state of the underneath implementation
   */
  bool Pull(vtkSMMessage*) override;

  /**
   * Parse the xml for the property.
   */
  bool ReadXMLAttributes(vtkSIProxy* proxyhelper, vtkPVXMLElement* element) override;

private:
  vtkSIStringVectorProperty(const vtkSIStringVectorProperty&) = delete;
  void operator=(const vtkSIStringVectorProperty&) = delete;

  class vtkVectorOfStrings;
  class vtkVectorOfInts;

  bool Push(const vtkVectorOfStrings& values);
  vtkVectorOfInts* ElementTypes;
  bool NeedReencoding;
};

#endif
