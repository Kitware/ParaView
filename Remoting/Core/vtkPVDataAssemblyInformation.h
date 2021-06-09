/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDataAssemblyInformation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkPVDataAssemblyInformation
 * @brief fetches vtkDataAssembly from a vtkObject subclass
 *
 * vtkPVDataAssemblyInformation is used to fetch vtkDataAssembly from a
 * vtkObject. The method used to obtain vtkDataAssembly instance is defined by
 * `MethodName` and defaults to "GetAssembly".
 */

#ifndef vtkPVDataAssemblyInformation_h
#define vtkPVDataAssemblyInformation_h

#include "vtkPVInformation.h"
#include "vtkRemotingCoreModule.h" //needed for exports

class vtkDataAssembly;

class VTKREMOTINGCORE_EXPORT vtkPVDataAssemblyInformation : public vtkPVInformation
{
public:
  static vtkPVDataAssemblyInformation* New();
  vtkTypeMacro(vtkPVDataAssemblyInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the method name to use to get the assembly.
   */
  vtkSetStringMacro(MethodName);
  vtkGetStringMacro(MethodName);
  ///@}

  vtkGetObjectMacro(DataAssembly, vtkDataAssembly);

  ///@{
  void CopyFromObject(vtkObject*) override;
  void CopyToStream(vtkClientServerStream*) override;
  void CopyFromStream(const vtkClientServerStream*) override;
  void CopyParametersToStream(vtkMultiProcessStream&) override;
  void CopyParametersFromStream(vtkMultiProcessStream&) override;
  ///@}

protected:
  vtkPVDataAssemblyInformation();
  ~vtkPVDataAssemblyInformation() override;

private:
  vtkPVDataAssemblyInformation(const vtkPVDataAssemblyInformation&) = delete;
  void operator=(const vtkPVDataAssemblyInformation&) = delete;

  void SetDataAssembly(vtkDataAssembly*);

  vtkDataAssembly* DataAssembly;
  char* MethodName;
};

#endif
