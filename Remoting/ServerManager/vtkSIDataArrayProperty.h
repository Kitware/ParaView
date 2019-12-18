/*=========================================================================

  Program:   ParaView
  Module:    vtkSIDataArrayProperty.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSIDataArrayProperty
 * @brief   InformationOnly property
 *
 * SIProperty that deals with vtkDataArray object type
*/

#ifndef vtkSIDataArrayProperty_h
#define vtkSIDataArrayProperty_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSIProperty.h"

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSIDataArrayProperty : public vtkSIProperty
{
public:
  static vtkSIDataArrayProperty* New();
  vtkTypeMacro(vtkSIDataArrayProperty, vtkSIProperty);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkSIDataArrayProperty();
  ~vtkSIDataArrayProperty() override;

  friend class vtkSIProxy;

  /**
   * Pull the current state of the underneath implementation
   */
  bool Pull(vtkSMMessage*) override;

private:
  vtkSIDataArrayProperty(const vtkSIDataArrayProperty&) = delete;
  void operator=(const vtkSIDataArrayProperty&) = delete;
};

#endif
