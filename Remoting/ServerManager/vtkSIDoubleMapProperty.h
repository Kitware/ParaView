/*=========================================================================

  Program:   ParaView
  Module:    vtkSIDoubleMapProperty.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSIDoubleMapProperty
 *
 * Map property that manage double value to be set through a method
 * on a vtkObject.
*/

#ifndef vtkSIDoubleMapProperty_h
#define vtkSIDoubleMapProperty_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSIProperty.h"

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSIDoubleMapProperty : public vtkSIProperty
{
public:
  static vtkSIDoubleMapProperty* New();
  vtkTypeMacro(vtkSIDoubleMapProperty, vtkSIProperty);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkGetStringMacro(CleanCommand) vtkSetStringMacro(CleanCommand)

    protected : vtkSIDoubleMapProperty();
  ~vtkSIDoubleMapProperty() override;

  bool Push(vtkSMMessage*, int) override;
  bool ReadXMLAttributes(vtkSIProxy* parent, vtkPVXMLElement* element) override;

  unsigned int NumberOfComponents;
  char* CleanCommand;

private:
  vtkSIDoubleMapProperty(const vtkSIDoubleMapProperty&) = delete;
  void operator=(const vtkSIDoubleMapProperty&) = delete;
};

#endif
