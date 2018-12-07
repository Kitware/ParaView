/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxyInitializationHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMProxyInitializationHelper
 * @brief   initialization helper for a proxy.
 *
 * vtkSMProxyInitializationHelper is used to add custom initialization logic to
 * the initialization of a proxy done by vtkSMParaViewPipelineController.
 * Developers can create new subclasses of vtkSMProxyInitializationHelper for
 * specific proxy types. vtkSMProxyInitializationHelper will instantiate the
 * helper and call PostInitializeProxy() in
 * vtkSMParaViewPipelineController::PostInitializeProxy().
 *
 * Helpers are added to a proxy in the XML configuration as follows:
 * \code{.xml}
 * <Proxy ...>
 *  <Hints>
 *    <InitializationHelper class="vtkMyCustomIntializationHelper" />
 *  </Hints>
 * </Proxy>
 * \endcode
*/

#ifndef vtkSMProxyInitializationHelper_h
#define vtkSMProxyInitializationHelper_h

#include "vtkSMObject.h"

class vtkPVXMLElement;
class vtkSMProxy;

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMProxyInitializationHelper : public vtkSMObject
{
public:
  vtkTypeMacro(vtkSMProxyInitializationHelper, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Called during vtkSMParaViewPipelineController::PostInitializeProxy() to
   * initialize a proxy.
   * @param proxy : the proxy being initialized.
   * @param xml : the XML configuration from this helper from this Hints for the
   * proxy. This makes it possible to pass additional configuration
   * parameters to the initialization helper.
   * @param initializationTimeStamp: the timestamp for the proxy initialization.
   * Generally, if a property on the proxy has MTime greater than
   * initializationTimeStamp, the initializer should not modify the
   * property since it was explicitly set by the user during
   * initialization.
   */
  virtual void PostInitializeProxy(
    vtkSMProxy* proxy, vtkPVXMLElement* xml, vtkMTimeType initializationTimeStamp) = 0;

protected:
  vtkSMProxyInitializationHelper();
  ~vtkSMProxyInitializationHelper() override;

private:
  vtkSMProxyInitializationHelper(const vtkSMProxyInitializationHelper&) = delete;
  void operator=(const vtkSMProxyInitializationHelper&) = delete;
};

#endif
