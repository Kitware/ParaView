// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMPlotlyJsonExtractWriterProxy.h"

#include "vtkObjectFactory.h"
#include "vtkPVXYChartView.h"
#include "vtkPlotlyJsonExporter.h"
#include "vtkSMContextViewProxy.h"
#include "vtkSMExtractsController.h"
#include "vtkSMPropertyHelper.h"

vtkStandardNewMacro(vtkSMPlotlyJsonExtractWriterProxy);
//----------------------------------------------------------------------------
vtkSMPlotlyJsonExtractWriterProxy::vtkSMPlotlyJsonExtractWriterProxy() = default;

//----------------------------------------------------------------------------
vtkSMPlotlyJsonExtractWriterProxy::~vtkSMPlotlyJsonExtractWriterProxy() = default;

//----------------------------------------------------------------------------
bool vtkSMPlotlyJsonExtractWriterProxy::CanExtract(vtkSMProxy* proxy)
{
  if (proxy)
  {
    vtkObjectBase* obj = proxy->GetClientSideObject();
    if (vtkPVXYChartView::SafeDownCast(obj))
    {
      return true;
    }
  }
  return false;
}

//----------------------------------------------------------------------------
bool vtkSMPlotlyJsonExtractWriterProxy::IsExtracting(vtkSMProxy* proxy)
{
  vtkSMPropertyHelper viewHelper(this, "View");
  return (viewHelper.GetAsProxy() == proxy);
}

//----------------------------------------------------------------------------
void vtkSMPlotlyJsonExtractWriterProxy::SetInput(vtkSMProxy* proxy)
{
  if (proxy == nullptr)
  {
    vtkErrorMacro("Input cannot be nullptr.");
    return;
  }

  vtkSMPropertyHelper viewHelper(this, "View");
  viewHelper.Set(proxy);
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMPlotlyJsonExtractWriterProxy::GetInput()
{
  return vtkSMPropertyHelper(this, "View").GetAsProxy();
}

//----------------------------------------------------------------------------
bool vtkSMPlotlyJsonExtractWriterProxy::Write(vtkSMExtractsController* extractor)
{
  auto fname = vtkSMPropertyHelper(this, "FileName").GetAsString();
  if (!fname)
  {
    vtkErrorMacro("Missing \"FileName\"!");
    return false;
  }

  // prefix the filename with the output directory
  auto convertedName =
    this->GenerateExtractsFileName(fname, extractor->GetRealExtractsOutputDirectory());

  auto view = vtkSMContextViewProxy::SafeDownCast(this->GetInput());
  if (!view)
  {
    vtkErrorMacro("No view provided to generate extract from!");
    return false;
  }
  bool status = false;
  if (auto contextView = vtkPVXYChartView::SafeDownCast(view->GetClientSideView()))
  {
    vtkNew<vtkPlotlyJsonExporter> exporter;
    exporter->SetFileName(convertedName.c_str());
    status = contextView->Export(exporter);
  }
  else
  {
    vtkErrorMacro("Provided view is not supported by vtkSMPlotlyJsonExtractWriterProxy");
  }
  if (status)
  {
    extractor->AddSummaryEntry(this, convertedName);
  }
  return status;
}

//----------------------------------------------------------------------------
void vtkSMPlotlyJsonExtractWriterProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
