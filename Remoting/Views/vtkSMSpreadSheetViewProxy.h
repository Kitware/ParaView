// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkSMSpreadSheetViewProxy
 * @brief proxy for "SpreadSheetView"
 *
 * vtkSMSpreadSheetRepresentationProxy extends vtkSMViewProxy to override a few
 * methods that add extra logic specific to this view. Namely, we override
 * `RepresentationVisibilityChanged` to ensure that the view picks a default
 * attribute type suitable for the visible representation.
 */

#ifndef vtkSMSpreadSheetViewProxy_h
#define vtkSMSpreadSheetViewProxy_h

#include "vtkRemotingViewsModule.h" // needed for exports
#include "vtkSMViewProxy.h"

class VTKREMOTINGVIEWS_EXPORT vtkSMSpreadSheetViewProxy : public vtkSMViewProxy
{
public:
  static vtkSMSpreadSheetViewProxy* New();
  vtkTypeMacro(vtkSMSpreadSheetViewProxy, vtkSMViewProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Overridden to pass a unique identifier to vtkSpreadSheetView.
   */
  void CreateVTKObjects() override;

  /**
   * Overridden to update `FieldAssociation` property on the view to match the
   * data type being shown.
   */
  void RepresentationVisibilityChanged(vtkSMProxy* repr, bool new_visibility) override;

protected:
  vtkSMSpreadSheetViewProxy();
  ~vtkSMSpreadSheetViewProxy() override;

private:
  vtkSMSpreadSheetViewProxy(const vtkSMSpreadSheetViewProxy&) = delete;
  void operator=(const vtkSMSpreadSheetViewProxy&) = delete;
};

#endif
