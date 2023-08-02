// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
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
