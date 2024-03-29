// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkSMRecolorableImageExtractWriterProxy
 * @brief extractor for recolorable images.
 *
 * vtkSMRecolorableImageExtractWriterProxy is an extractors that saves out
 * recolorable images instead of standard PNGs from views. Recolorable images
 * are obtained from vtkPVRenderView by rendering and capturing the raw data
 * array values instead of mapping them through a lookup table. The
 * implementation uses vtkValuePass.
 *
 * Currently, this is only supported in client-only (builtin) and parallel
 * pvbatch (or insitu) modes. Any client-server configuration is not supported.
 */

#ifndef vtkSMRecolorableImageExtractWriterProxy_h
#define vtkSMRecolorableImageExtractWriterProxy_h

#include "vtkSMImageExtractWriterProxy.h"

class VTKREMOTINGVIEWS_EXPORT vtkSMRecolorableImageExtractWriterProxy
  : public vtkSMImageExtractWriterProxy
{
public:
  static vtkSMRecolorableImageExtractWriterProxy* New();
  vtkTypeMacro(vtkSMRecolorableImageExtractWriterProxy, vtkSMImageExtractWriterProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void CreateVTKObjects() override;

protected:
  vtkSMRecolorableImageExtractWriterProxy();
  ~vtkSMRecolorableImageExtractWriterProxy() override;

  bool WriteInternal(vtkSMExtractsController* extractor,
    const SummaryParametersT& params = SummaryParametersT{}) override;

private:
  vtkSMRecolorableImageExtractWriterProxy(const vtkSMRecolorableImageExtractWriterProxy&) = delete;
  void operator=(const vtkSMRecolorableImageExtractWriterProxy&) = delete;
};

#endif
