// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMTimeStepsDomain
 * @brief   A domain providing timesteps from its "Input" property
 *
 * vtkSMTimeStepsDomain recovers input timesteps fromg its "Input" vtkSMInputProerty
 * using vtkPVDataInformation API.
 *
 * @sa vtkSMTimeStepIndexDomain
 *
 */

#ifndef vtkSMTimeStepsDomain_h
#define vtkSMTimeStepsDomain_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSMDomain.h"

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMTimeStepsDomain : public vtkSMDomain
{
public:
  static vtkSMTimeStepsDomain* New();
  vtkTypeMacro(vtkSMTimeStepsDomain, vtkSMDomain)

  /**
   * Recovers timesteps from a required property "Input" and
   * store them into the internal Values.
   */
  void Update(vtkSMProperty* prop) override;

  /**
   * Returns the vector of values containing the timesteps after Update.
   */
  const std::vector<double>& GetValues() { return this->Values; };

  /**
   * Returns the value at index from the timesteps after Update.
   */
  double GetValue(unsigned int i) { return this->Values[i]; };

  /**
   * Returns the number of values containing the timesteps after Update.
   */
  unsigned int GetNumberOfValues() { return static_cast<unsigned int>(this->Values.size()); };

protected:
  vtkSMTimeStepsDomain() = default;
  ~vtkSMTimeStepsDomain() override = default;

private:
  vtkSMTimeStepsDomain(const vtkSMTimeStepsDomain&) = delete;
  void operator=(const vtkSMTimeStepsDomain&) = delete;

  std::vector<double> Values;
};

#endif // vtkSMTimeStepsDomain_h
