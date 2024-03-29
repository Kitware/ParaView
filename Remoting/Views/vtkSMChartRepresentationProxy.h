// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMChartRepresentationProxy
 *
 *
 */

#ifndef vtkSMChartRepresentationProxy_h
#define vtkSMChartRepresentationProxy_h

#include "vtkRemotingViewsModule.h" //needed for exports
#include "vtkSMRepresentationProxy.h"

class vtkChartRepresentation;

class VTKREMOTINGVIEWS_EXPORT vtkSMChartRepresentationProxy : public vtkSMRepresentationProxy
{
public:
  static vtkSMChartRepresentationProxy* New();
  vtkTypeMacro(vtkSMChartRepresentationProxy, vtkSMRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Returns client side representation object.
   */
  vtkChartRepresentation* GetRepresentation();

  /**
   * Overridden to handle links with subproxy properties.
   */
  int ReadXMLAttributes(vtkSMSessionProxyManager* pm, vtkPVXMLElement* element) override;

protected:
  vtkSMChartRepresentationProxy();
  ~vtkSMChartRepresentationProxy() override;

  /**
   * Overridden to ensure that whenever "Input" property changes, we update the
   * "Input" properties for all internal representations (including setting up
   * of the link to the extract-selection representation).
   * Two selection input properties are available. The standard one, created by
   * vtkPVExtractSelection, is named "SelectionInput". The other one, which is just the
   * original input selection, is named "OriginalSelectionInput".
   * see views_and_representations.xml::HistogramChartRepresentation::SelectionRepresentation
   * for an example usage.
   */
  void SetPropertyModifiedFlag(const char* name, int flag) override;

private:
  vtkSMChartRepresentationProxy(const vtkSMChartRepresentationProxy&) = delete;
  void operator=(const vtkSMChartRepresentationProxy&) = delete;
};

#endif
