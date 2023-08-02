// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVCatalystChannelInformation
 * @brief   Find out the Catalyst input channel name for a dataset.
 *
 * Find out the Catalyst input channel name for a dataset.
 */

#ifndef vtkPVCatalystChannelInformation_h
#define vtkPVCatalystChannelInformation_h

#include "vtkPVInformation.h"
#include "vtkRemotingServerManagerModule.h" //needed for exports
#include <string>                           // for ChannelName member variable

class VTKREMOTINGSERVERMANAGER_EXPORT vtkPVCatalystChannelInformation : public vtkPVInformation
{
public:
  static vtkPVCatalystChannelInformation* New();
  vtkTypeMacro(vtkPVCatalystChannelInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Get the channel name.
   */
  vtkGetMacro(ChannelName, std::string);

  /**
   * Transfer information about a single object into this object.
   */
  void CopyFromObject(vtkObject*) override;

  /**
   * Merge another information object, it really just overwrites the information
   * if it exists in the new information input.
   */
  void AddInformation(vtkPVInformation* other) override;

  ///@{
  /**
   * Manage a serialized version of the information.
   */
  void CopyToStream(vtkClientServerStream*) override;
  void CopyFromStream(const vtkClientServerStream*) override;
  ///@}

  /**
   * Remove all gathered information.
   */
  void Initialize();

protected:
  vtkPVCatalystChannelInformation();
  ~vtkPVCatalystChannelInformation() override;

  /// Information results
  ///@{
  std::string ChannelName;
  ///@}

  vtkPVCatalystChannelInformation(const vtkPVCatalystChannelInformation&) = delete;
  void operator=(const vtkPVCatalystChannelInformation&) = delete;
};

#endif
