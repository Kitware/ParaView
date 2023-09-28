// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkSMPlotlyJsonExtractWriterProxy
 * @brief extractor used to extract data from certain views to a json file
 * folowing the plotly-json schema.
 *
 * Currently, we support vtkPVXYChartView (only line plots for now but could be
 * extended in the future for other kinds of plots Bar etc.)
 *
 * @sa vtkPlotlyJsonExporter
 */

#ifndef vtkSMPlotlyJsonExtractWriterProxy_h
#define vtkSMPlotlyJsonExtractWriterProxy_h

#include "vtkRemotingViewsModule.h"  // needed for exports
#include "vtkSMExtractWriterProxy.h" // base class
#include "vtkSMViewProxy.h"          // for member variable

class vtkSMViewProxy;

class VTKREMOTINGVIEWS_EXPORT vtkSMPlotlyJsonExtractWriterProxy : public vtkSMExtractWriterProxy
{
public:
  static vtkSMPlotlyJsonExtractWriterProxy* New();
  vtkTypeMacro(vtkSMPlotlyJsonExtractWriterProxy, vtkSMExtractWriterProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set the view proxy to export.
   */
  vtkSetObjectMacro(View, vtkSMViewProxy);
  vtkGetObjectMacro(View, vtkSMViewProxy);
  ///@}

  ///@{
  /**
   * Implementation for vtkSMExtractWriterProxy API.
   */
  bool Write(vtkSMExtractsController* extractor) override;
  bool CanExtract(vtkSMProxy* proxy) override;
  bool IsExtracting(vtkSMProxy* proxy) override;
  void SetInput(vtkSMProxy* proxy) override;
  vtkSMProxy* GetInput() override;
  ///@}
protected:
  vtkSMPlotlyJsonExtractWriterProxy();
  ~vtkSMPlotlyJsonExtractWriterProxy() override;

private:
  vtkSMPlotlyJsonExtractWriterProxy(const vtkSMPlotlyJsonExtractWriterProxy&) = delete;
  void operator=(const vtkSMPlotlyJsonExtractWriterProxy&) = delete;

  vtkSMViewProxy* View = nullptr;
};

#endif
