// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMAnimatedExporterProxy
 * @brief   used to export the animated view
 *
 * It is intended to be associated to a vtkSMAnimationSceneWriter subclass.
 *
 * In the particular case of vtkSMAnimationSceneSeriesWriter, it expects
 * an "Exporter" SubProxy pointing to an ExporterProxy that will be used
 * to export each animation scene timestep.
 * @sa
 */

#ifndef vtkSMAnimatedExporterProxy_h
#define vtkSMAnimatedExporterProxy_h

#include "vtkRemotingAnimationModule.h" //needed for exports
#include "vtkSMExporterProxy.h"
#include "vtkSMViewProxy.h"

class VTKREMOTINGANIMATION_EXPORT vtkSMAnimatedExporterProxy : public vtkSMExporterProxy
{
public:
  static vtkSMAnimatedExporterProxy* New();
  vtkTypeMacro(vtkSMAnimatedExporterProxy, vtkSMExporterProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Exports the view.
   */
  void Write() override;

  /**
   * Set the view to export.
   * Reimplemented to forward it to the Exporter SubProxy.
   */
  void SetView(vtkSMViewProxy* view) override;

  /**
   * Returns true if the view can be exported.
   */
  bool CanExport(vtkSMProxy*) override;

protected:
  vtkSMAnimatedExporterProxy() = default;
  ~vtkSMAnimatedExporterProxy() override = default;

private:
  vtkSMAnimatedExporterProxy(const vtkSMAnimatedExporterProxy&) = delete;
  void operator=(const vtkSMAnimatedExporterProxy&) = delete;
};

#endif
