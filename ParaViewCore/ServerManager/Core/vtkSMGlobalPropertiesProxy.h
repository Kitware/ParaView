/*=========================================================================

  Program:   ParaView
  Module:    vtkSMGlobalPropertiesProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMGlobalPropertiesProxy
 * @brief   proxy that adds support for linking
 * properties with other proxies designed for use-cases like color-palettes.
 *
 * vtkSMGlobalPropertiesProxy can be thought of as a proxy which provides API to
 * link its properties with any arbitrary proxy unidirectionally, so that if the
 * property value on this proxy changes, the linked property on every other
 * proxy is updated. However if the linked property (or target property) is
 * modified externally, then the link is automatically broken.
 *
 * This is suitable adding ability for color palettes. Using XML hints in the
 * proxy definition, one can write application code that setups links with a
 * proxy and the color palette proxy (as done in
 * vtkSMParaViewPipelineController). Now as long as the user doesn't modify
 * the linked properties, the color palette can be changed and it will reflect
 * on all linked properties.
 *
 * vtkSMParaViewPipelineController uses the property level hint
 * \c \<GlobalPropertyLink/\> to define such links e.g.
 *    \code{.xml}
 *    <DoubleVectorProperty name="Background" ... >
 *         ...
 *      <Hints>
 *        <GlobalPropertyLink type="ColorPalette" property="BackgroundColor" />
 *      </Hints>
 *    </DoubleVectorProperty>
 *    \endcode
 *
 * While vtkSMParaViewPipelineController currently only respects
 * hints on the property, we can in future add support for respecting hints at
 * the proxy level if needed.
*/

#ifndef vtkSMGlobalPropertiesProxy_h
#define vtkSMGlobalPropertiesProxy_h

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkSMProxy.h"

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMGlobalPropertiesProxy : public vtkSMProxy
{
public:
  static vtkSMGlobalPropertiesProxy* New();
  vtkTypeMacro(vtkSMGlobalPropertiesProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Set up a property link with the given property on the proxy with the
   * specified property on this proxy. The link is automatically broken if the
   * target property is modified outside by someone other than this
   * vtkSMGlobalPropertiesProxy instance or when the targetProxy is destroyed.
   */
  bool Link(const char* propertyname, vtkSMProxy* targetProxy, const char* targetPropertyName);

  /**
   * Unlink a property link,
   */
  bool Unlink(const char* propertyname, vtkSMProxy* targetProxy, const char* targetPropertyName);

  /**
   * Remove all links.
   */
  void RemoveAllLinks();

  /**
   * If a link between the target and a property on this proxy exists, returns
   * the name of that property else NULL.
   */
  const char* GetLinkedPropertyName(vtkSMProxy* targetProxy, const char* targetPropertyName);

  //@{
  /**
   * Overridden to save link state.
   */
  vtkPVXMLElement* SaveXMLState(vtkPVXMLElement* root, vtkSMPropertyIterator* iter) override;
  using Superclass::SaveXMLState;
  //@}

  /**
   * Overridden to load links state.
   */
  int LoadXMLState(vtkPVXMLElement* element, vtkSMProxyLocator* locator) override;

protected:
  vtkSMGlobalPropertiesProxy();
  ~vtkSMGlobalPropertiesProxy() override;

  /**
   * Overridden to propagate the modification to the linked properties.
   */
  void SetPropertyModifiedFlag(const char* name, int flag) override;

  /**
   * Called when a target properties is modified.
   */
  void TargetPropertyModified(vtkObject*, unsigned long, void*);

private:
  vtkSMGlobalPropertiesProxy(const vtkSMGlobalPropertiesProxy&) = delete;
  void operator=(const vtkSMGlobalPropertiesProxy&) = delete;

  class vtkInternals;
  vtkInternals* Internals;
};

#endif
