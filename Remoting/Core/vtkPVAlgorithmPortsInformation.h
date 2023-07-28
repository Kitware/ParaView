// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
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

#include "vtkPVInformation.h"
#include "vtkRemotingCoreModule.h" //needed for exports

class VTKREMOTINGCORE_EXPORT vtkPVAlgorithmPortsInformation : public vtkPVInformation
{
public:
  static vtkPVAlgorithmPortsInformation* New();
  vtkTypeMacro(vtkPVAlgorithmPortsInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get number of outputs for a particular source.
   */
  vtkGetMacro(NumberOfOutputs, int);
  ///@}

  ///@{
  /**
   * Get the number of required inputs for a particular algorithm.
   */
  vtkGetMacro(NumberOfRequiredInputs, int);
  ///@}

  /**
   * Transfer information about a single object into this object.
   */
  void CopyFromObject(vtkObject*) override;

  /**
   * Merge another information object.
   */
  void AddInformation(vtkPVInformation*) override;

  ///@{
  /**
   * Manage a serialized version of the information.
   */
  void CopyToStream(vtkClientServerStream*) override;
  void CopyFromStream(const vtkClientServerStream*) override;
  ///@}

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
