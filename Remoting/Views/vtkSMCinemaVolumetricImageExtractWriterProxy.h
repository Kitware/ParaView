/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCinemaVolumetricImageExtractWriterProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
