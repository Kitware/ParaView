// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkPVTemporalDataInformation
 * @brief extends vtkPVDataInformation to gather information across timesteps.
 *
 * vtkPVTemporalDataInformation is used to gather data information over time.
 * It simply overrides `vtkPVDataInformation::CopyFromObject` to ensure that the
 * data information is collected from all timesteps and not just 1.
 */

#ifndef vtkPVTemporalDataInformation_h
#define vtkPVTemporalDataInformation_h

#include "vtkPVDataInformation.h"
#include "vtkRemotingCoreModule.h" //needed for exports

class vtkPVArrayInformation;
class vtkPVDataSetAttributesInformation;

class VTKREMOTINGCORE_EXPORT vtkPVTemporalDataInformation : public vtkPVDataInformation
{
public:
  static vtkPVTemporalDataInformation* New();
  vtkTypeMacro(vtkPVTemporalDataInformation, vtkPVDataInformation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Transfer information about a single object into this object.
   * This expects the \c object to be a vtkAlgorithmOutput.
   */
  void CopyFromObject(vtkObject* object) override;

protected:
  vtkPVTemporalDataInformation();
  ~vtkPVTemporalDataInformation() override;

private:
  vtkPVTemporalDataInformation(const vtkPVTemporalDataInformation&) = delete;
  void operator=(const vtkPVTemporalDataInformation&) = delete;
};

#endif
