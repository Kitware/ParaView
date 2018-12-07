/*=========================================================================

  Program:   ParaView
  Module:    vtkPVAlgorithmPortsInformation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVAlgorithmPortsInformation
 * @brief   Holds number of outputs
 *
 * This information object collects the number of outputs from the
 * sources.  This is separate from vtkPVDataInformation because the number of
 * outputs can be determined before Update is called.
*/

#ifndef vtkPVAlgorithmPortsInformation_h
#define vtkPVAlgorithmPortsInformation_h

#include "vtkPVClientServerCoreCoreModule.h" //needed for exports
#include "vtkPVInformation.h"

class VTKPVCLIENTSERVERCORECORE_EXPORT vtkPVAlgorithmPortsInformation : public vtkPVInformation
{
public:
  static vtkPVAlgorithmPortsInformation* New();
  vtkTypeMacro(vtkPVAlgorithmPortsInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get number of outputs for a particular source.
   */
  vtkGetMacro(NumberOfOutputs, int);
  //@}

  //@{
  /**
   * Get the number of required inputs for a particular algorithm.
   */
  vtkGetMacro(NumberOfRequiredInputs, int);
  //@}

  /**
   * Transfer information about a single object into this object.
   */
  void CopyFromObject(vtkObject*) override;

  /**
   * Merge another information object.
   */
  void AddInformation(vtkPVInformation*) override;

  //@{
  /**
   * Manage a serialized version of the information.
   */
  void CopyToStream(vtkClientServerStream*) override;
  void CopyFromStream(const vtkClientServerStream*) override;
  //@}

protected:
  vtkPVAlgorithmPortsInformation();
  ~vtkPVAlgorithmPortsInformation() override;

  int NumberOfOutputs;
  int NumberOfRequiredInputs;

  vtkSetMacro(NumberOfOutputs, int);

private:
  vtkPVAlgorithmPortsInformation(const vtkPVAlgorithmPortsInformation&) = delete;
  void operator=(const vtkPVAlgorithmPortsInformation&) = delete;
};

#endif
