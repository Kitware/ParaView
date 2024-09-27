// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkSMBivariateTextureRepresentationProxy
 * @brief Proxy for the vtkBivaritateTextureRepresentation
 *
 * The vtkBivariateTextureRepresentation comes along with a dedicated textured 2D scalar bar
 * actor. Unlike other other scalar bars in ParaView, this one is not associated to a LUT
 * but to the representation itself. An unique instance of the scalar bar actor is created
 * for each representation proxy instance and is used throughout the proxy lifetime.
 *
 * The vtkSMBivariateTextureRepresentationProxy manages the links between the properties of
 * the bivariate texture representation and the associated scalar bar (texture, axes, titles).
 *
 * The vtkSMBivariateTextureRepresentationProxy is also responsible to show or hide the textured
 * scalar bar in the view, since this cannot be handled through the standard mechanisms based on
 * the active LUTs.
 *
 * @sa vtkBivariateTextureRepresentation
 */

#ifndef vtkSMBivariateTextureRepresentationProxy_h
#define vtkSMBivariateTextureRepresentationProxy_h

#include "vtkBivariateRepresentationsModule.h" // for export macro
#include "vtkSMProxy.h"                        // for API
#include "vtkSMRepresentationProxy.h"          // for API
#include "vtkSmartPointer.h"                   // for API
#include "vtkWeakPointer.h"                    // for API

#include <vector> // for API

class vtkSMLink;

class VTKBIVARIATEREPRESENTATIONS_EXPORT vtkSMBivariateTextureRepresentationProxy
  : public vtkSMRepresentationProxy
{
public:
  static vtkSMBivariateTextureRepresentationProxy* New();
  vtkTypeMacro(vtkSMBivariateTextureRepresentationProxy, vtkSMRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Called after the view updates.
   */
  void ViewUpdated(vtkSMProxy* view) override;

protected:
  vtkSMBivariateTextureRepresentationProxy() = default;
  ~vtkSMBivariateTextureRepresentationProxy() override = default;

  /**
   * Called each time the internal representation of the composite representation is
   * changed (the bivariate texture representation being one of them).
   * In addition to instanciate the VTK objects if needed, takes care to unregister
   * the custom textured scalar bar proxy to avoid other internal representations
   * to use it when switching back to one of them.
   */
  void CreateVTKObjects() override;

  /**
   * Keep synchronised the representation with the scalar bar.
   */
  void OnPropertyUpdated(vtkObject*, unsigned long, void* calldata);

private:
  vtkSMBivariateTextureRepresentationProxy(
    const vtkSMBivariateTextureRepresentationProxy&) = delete;
  void operator=(const vtkSMBivariateTextureRepresentationProxy&) = delete;

  /**
   * Setup all links between properties and informations. It also update properties
   * information before anything else. Usually called in CreateVTKObject() function.
   */
  void SetupPropertiesLinks();

  /**
   * Show the associated textured scalar bar in the current view.
   */
  void ShowTexturedScalarBar();

  /**
   * Create the associated textured scalar bar.
   */
  void CreateTexturedScalarBar();

  vtkSmartPointer<vtkSMProxy> TexturedScalarBarProxy;
  vtkWeakPointer<vtkSMProxy> CurrentView;
  bool UpdatingPropertyInfo = false;

  typedef std::vector<vtkSmartPointer<vtkSMLink>> LinksType;
  LinksType Links;
};

#endif
