/*=========================================================================

  Program:   ParaView
  Module:    vtkPVGenericAttributeInformation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVGenericAttributeInformation
 * @brief   Generic attribute information like type.
 *
 * This objects is for eliminating direct access to vtkDataObjects
 * by the "client".  Only vtkPVPart and vtkPVProcessModule should access
 * the data directly.  At the moment, this object is only a container
 * and has no useful methods for operating on data.
*/

#ifndef vtkPVGenericAttributeInformation_h
#define vtkPVGenericAttributeInformation_h

#include "vtkPVArrayInformation.h"
#include "vtkPVClientServerCoreCoreModule.h" //needed for exports

class vtkClientServerStream;

class VTKPVCLIENTSERVERCORECORE_EXPORT vtkPVGenericAttributeInformation
  : public vtkPVArrayInformation
{
public:
  static vtkPVGenericAttributeInformation* New();
  vtkTypeMacro(vtkPVGenericAttributeInformation, vtkPVArrayInformation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Transfer information about a single object into this object.
   */
  void CopyFromObject(vtkObject*) override;

protected:
  vtkPVGenericAttributeInformation();
  ~vtkPVGenericAttributeInformation() override;

  vtkPVGenericAttributeInformation(const vtkPVGenericAttributeInformation&) = delete;
  void operator=(const vtkPVGenericAttributeInformation&) = delete;
};

#endif
