// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkPVLogInformation_h
#define vtkPVLogInformation_h

#include "vtkPVInformation.h"
#include "vtkRemotingCoreModule.h" // needed for exports

#include <string> // for std::string

class vtkClientServerStream;

/**
 * @class vtkPVLogInformation
 * @brief Gets the log of a specific rank as well as the verbosity level
 */
class VTKREMOTINGCORE_EXPORT vtkPVLogInformation : public vtkPVInformation
{
public:
  static vtkPVLogInformation* New();
  vtkTypeMacro(vtkPVLogInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

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

  ///@{
  /**
   * Serialize/Deserialize the parameters that control how/what information is
   * gathered. This are different from the ivars that constitute the gathered
   * information itself. For example, PortNumber on vtkPVDataInformation
   * controls what output port the data-information is gathered from.
   */
  void CopyParametersToStream(vtkMultiProcessStream&) override;
  void CopyParametersFromStream(vtkMultiProcessStream&) override;
  ///@}

  /**
   * Get the recorded logs.
   */
  vtkGetMacro(Logs, const std::string&);

  /**
   * Get the starting log.
   */
  vtkGetMacro(StartingLogs, const std::string&);

  /**
   * Set the rank to get log from.
   */
  vtkSetMacro(Rank, int);

  /**
   * Get the verbosity level of the server.
   */
  vtkGetMacro(Verbosity, int);

protected:
  vtkPVLogInformation() = default;
  ~vtkPVLogInformation() override = default;

  int Rank = -1;
  std::string Logs;
  std::string StartingLogs;
  int Verbosity = 20;

  vtkPVLogInformation(const vtkPVLogInformation&) = delete;
  void operator=(const vtkPVLogInformation&) = delete;
};

#endif // vtkPVLogInformation_h
