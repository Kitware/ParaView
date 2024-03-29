// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkSMOMETIFFChannelCalculatorInitializationHelper
 * @brief initialization helper for "OMETIFFChannelCalculator" filter.
 *
 * vtkSMOMETIFFChannelCalculatorInitializationHelper is an initialization helper
 * for "OMETIFFChannelCalculator" which helps setup the transfer functions for
 * all the channels.
 *
 */

#ifndef vtkSMOMETIFFChannelCalculatorInitializationHelper_h
#define vtkSMOMETIFFChannelCalculatorInitializationHelper_h

#include "vtkRemotingMiscModule.h" //needed for exports
#include "vtkSMProxyInitializationHelper.h"

class VTKREMOTINGMISC_EXPORT vtkSMOMETIFFChannelCalculatorInitializationHelper
  : public vtkSMProxyInitializationHelper
{
public:
  static vtkSMOMETIFFChannelCalculatorInitializationHelper* New();
  vtkTypeMacro(vtkSMOMETIFFChannelCalculatorInitializationHelper, vtkSMProxyInitializationHelper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void PostInitializeProxy(vtkSMProxy*, vtkPVXMLElement*, vtkMTimeType) override;
  void RegisterProxy(vtkSMProxy* proxy, vtkPVXMLElement* xml) override;

protected:
  vtkSMOMETIFFChannelCalculatorInitializationHelper();
  ~vtkSMOMETIFFChannelCalculatorInitializationHelper() override;

private:
  vtkSMOMETIFFChannelCalculatorInitializationHelper(
    const vtkSMOMETIFFChannelCalculatorInitializationHelper&) = delete;
  void operator=(const vtkSMOMETIFFChannelCalculatorInitializationHelper&) = delete;
};

#endif
