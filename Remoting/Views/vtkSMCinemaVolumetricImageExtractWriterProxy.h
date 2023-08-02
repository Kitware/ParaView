// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkSMCinemaVolumetricImageExtractWriterProxy
 * @brief image extractor for cinema-volumetric extracts
 *
 * vtkSMCinemaVolumetricImageExtractWriterProxy subclasses
 * vtkSMImageExtractWriterProxy to add ability to export transfer functions for
 * volumetric renderings.
 *
 * @note This is an experimental feature and may change without notice.
 */

#ifndef vtkSMCinemaVolumetricImageExtractWriterProxy_h
#define vtkSMCinemaVolumetricImageExtractWriterProxy_h

#include "vtkSMImageExtractWriterProxy.h"

class VTKREMOTINGVIEWS_EXPORT vtkSMCinemaVolumetricImageExtractWriterProxy
  : public vtkSMImageExtractWriterProxy
{
public:
  static vtkSMCinemaVolumetricImageExtractWriterProxy* New();
  vtkTypeMacro(vtkSMCinemaVolumetricImageExtractWriterProxy, vtkSMImageExtractWriterProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkSMCinemaVolumetricImageExtractWriterProxy();
  ~vtkSMCinemaVolumetricImageExtractWriterProxy() override;

  bool WriteInternal(vtkSMExtractsController* extractor,
    const SummaryParametersT& params = SummaryParametersT{}) override;
  const char* GetShortName(const std::string& key) const override;

private:
  vtkSMCinemaVolumetricImageExtractWriterProxy(
    const vtkSMCinemaVolumetricImageExtractWriterProxy&) = delete;
  void operator=(const vtkSMCinemaVolumetricImageExtractWriterProxy&) = delete;
};

#endif
