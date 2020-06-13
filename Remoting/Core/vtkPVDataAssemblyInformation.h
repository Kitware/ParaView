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
 * @brief information to collect vtkDataAssembly
 *
 * vtkPVDataAssemblyInformation gathers vtkDataAssembly from an algorithm's
 * output port (chosen using vtkPVDataAssemblyInformation::SetPortNumber).
 * This is a root-only data information, i.e. it only collects the information
 * from the root node since all ranks are expected to have identical
 * vtkDataAssembly, if any.
 */

#ifndef vtkPVDataAssemblyInformation_h
#define vtkPVDataAssemblyInformation_h

#include "vtkPVInformation.h"
#include "vtkRemotingCoreModule.h" //needed for exports
#include "vtkSmartPointer.h"       // for vtkSmartPointer

class vtkDataAssembly;
class VTKREMOTINGCORE_EXPORT vtkPVDataAssemblyInformation : public vtkPVInformation
{
public:
  static vtkPVDataAssemblyInformation* New();
  vtkTypeMacro(vtkPVDataAssemblyInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void Initialize();

  //@{
  /**
   * Port number controls which output port the information is gathered from.
   * This is the only parameter that can be set on  the client-side before
   * gathering the information.
   */
  vtkSetMacro(PortNumber, int);
  vtkGetMacro(PortNumber, int);
  //@}

  /**
   * Transfer information about a single object into this object.
   */
  void CopyFromObject(vtkObject*) override;

  //@{
  /**
   * Manage a serialized version of the information.
   */
  void CopyToStream(vtkClientServerStream*) override;
  void CopyFromStream(const vtkClientServerStream*) override;
  void CopyParametersToStream(vtkMultiProcessStream&) override;
  void CopyParametersFromStream(vtkMultiProcessStream&) override;
  //@}

  /**
   * Returns the gathered vtkDataAssembly.
   */
  vtkDataAssembly* GetDataAssembly();

protected:
  vtkPVDataAssemblyInformation();
  ~vtkPVDataAssemblyInformation();

private:
  vtkPVDataAssemblyInformation(const vtkPVDataAssemblyInformation&) = delete;
  void operator=(const vtkPVDataAssemblyInformation&) = delete;

  vtkSmartPointer<vtkDataAssembly> DataAssembly;
  int PortNumber;
};

#endif
