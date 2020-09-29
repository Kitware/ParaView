/*=========================================================================

  Program:   ParaView
  Module:    vtkSMParaViewPipelineController.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkSMParaViewPipelineController
 * @brief Controller that encapsulates control logic for typical ParaView
 * applications.
 *
 * ParaView's ServerManager, together with it's proxies and properties provides
 * a mechanism to create visualization and data processing pipelines. However,
 * for a complex application like ParaView, there is considerable control logic
 * to manage these proxies, set them up, etc. for common operations supported by
 * the application. vtkSMParaViewPipelineController provides us a mechanism to
 * encapsulate just control logic with ability to customize and extend for
 * custom applications, similar to ParaView.
 *
 * vtkSMParaViewPipelineController has no state of itself. When needed, one
 * should simply create an instance of vtkSMParaViewPipelineController and
 * release it once done.
 *
 * vtkSMParaViewPipelineController uses the vtkObjectFactory mechanism.
 * Custom application developers can provide subclasses and override the
 * behaviour (see vtkSMParaViewPipelineControllerWithRender).
 *
 * For an example of using vtkSMParaViewPipelineController in your application,
 * see `Remoting/Views/Testing/Cxx/TestParaViewPipelineController.cxx`.
 *
*/

#ifndef vtkSMParaViewPipelineController_h
#define vtkSMParaViewPipelineController_h

#include "vtkSMObject.h"

#include <string>

class vtkSMProxy;
class vtkSMSession;
class vtkSMSessionProxyManager;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMParaViewPipelineController : public vtkSMObject
{
public:
  static vtkSMParaViewPipelineController* New();
  vtkTypeMacro(vtkSMParaViewPipelineController, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Call this method to setup a branch new session with state considered
   * essential for ParaView session. Returns true on success.
   */
  virtual bool InitializeSession(vtkSMSession* session);

  /**
   * Returns the TimeKeeper proxy associated with the session.
   */
  virtual vtkSMProxy* FindTimeKeeper(vtkSMSession* session);

  /**
   * Returns the MaterialLibrary proxy associated with the session.
   */
  virtual vtkSMProxy* FindMaterialLibrary(vtkSMSession* session);

  //---------------------------------------------------------------------------
  /**
   * Pre-initializes a proxy i.e. prepares the proxy for initialization.
   * One should call this before changing any properties on the proxy. We load
   * the property values from XML defaults as well as user-preferences here.
   */
  virtual bool PreInitializeProxy(vtkSMProxy* proxy);

  /**
   * Final step in proxy initialization. When this method is called, all
   * essential properties on the proxy (such as inputs for filters, or filename
   * on readers) are assumed to be set up so that domains can be updated. This
   * method setups up property values for those properties that weren't modified
   * since the PreInitializeProxy() using the domains, if possible. This enables
   * the application to select data-specific default values.
   * NOTE: This method does not register the proxy with the proxy manager. It
   * may, however register any helper proxies created for this proxy.
   */
  virtual bool PostInitializeProxy(vtkSMProxy* proxy);

  /**
   * Convenience method to call PreInitializeProxy and PostInitializeProxy.
   */
  bool InitializeProxy(vtkSMProxy* proxy)
  {
    return this->PreInitializeProxy(proxy) && this->PostInitializeProxy(proxy);
  }

  /**
   * Cleans up any helper proxies registered for the proxy in
   * PreInitializeProxy/PostInitializeProxy. Similar to
   * PreInitializeProxy/PostInitializeProxy methods, this doesn't affect the
   * proxy manager registration state for the proxy itself.
   */
  virtual bool FinalizeProxy(vtkSMProxy* proxy);

  //---------------------------------------------------------------------------
  // ******* Methods for Pipeline objects like sources/filters/readers ********

  /**
   * Use this method after PreInitializeProxy() and PostInitializeProxy() to
   * register a pipeline proxy with the proxy manager. This method does
   * additional updates required for pipeline proxies such as registering the
   * proxy with the TimeKeeper, creating additional helper proxies for enabling
   * representation animations, and updating the active-source. This method will
   * register the proxy in an appropriate group so that the application
   * becomes aware of it. One can optionally pass in the registration name to
   * use. Otherwise, this code will come up with a unique name.
   * Caveat: while pipeline proxies are generally registered under the "sources"
   * group, there's one exception: sources that produce vtkSelection. ParaView
   * treats them specially and registers them under "selection_sources".
   */
  virtual bool RegisterPipelineProxy(vtkSMProxy* proxy, const char* proxyname);
  virtual bool RegisterPipelineProxy(vtkSMProxy* proxy)
  {
    return this->RegisterPipelineProxy(proxy, NULL);
  }

  /**
   * Unregisters a pipeline proxy. This is the inverse of RegisterPipelineProxy()
   * and hence unsets the active source if the active source if this proxy,
   * unregisters the proxy with the TimeKeeper etc.
   * Users can use either this method or the catch-all
   * vtkSMParaViewPipelineController::UnRegisterProxy() method which
   * determines the type of the proxy and then calls the appropriate method.
   */
  virtual bool UnRegisterPipelineProxy(vtkSMProxy* proxy);

  //---------------------------------------------------------------------------
  // *******  Methods for Views/Displays *********

  /**
   * Use this method after PreInitializeProxy() and PostInitializeProxy() to
   * register a view proxy with the proxy manager. This will also perform any
   * additional setups as needed e.g. registering the view with the
   * animation scene and the timer keeper.
   */
  virtual bool RegisterViewProxy(vtkSMProxy* proxy) { return this->RegisterViewProxy(proxy, NULL); }
  virtual bool RegisterViewProxy(vtkSMProxy* proxy, const char* proxyname);

  /**
   * Inverse of RegisterViewProxy.
   * Users can use either this method or the catch-all
   * vtkSMParaViewPipelineController::UnRegisterProxy() method which
   * determines the type of the proxy and then calls the appropriate method.
   * If the optional argument, \c unregister_representations, is false (default
   * is true), then this method will skip the unregistering of representations.
   * Default behaviour is to unregister all representations too.
   */
  virtual bool UnRegisterViewProxy(vtkSMProxy* proxy, bool unregister_representations = true);

  /**
   * Registration method for representations to be used after
   * PreInitializeProxy() and PostInitializeProxy(). Register the proxy under
   * the appropriate group.
   */
  virtual bool RegisterRepresentationProxy(vtkSMProxy* proxy);

  /**
   * Unregisters a representation proxy.
   * Users can use either this method or the catch-all
   * vtkSMParaViewPipelineController::UnRegisterProxy() method which
   * determines the type of the proxy and then calls the appropriate method.
   */
  virtual bool UnRegisterRepresentationProxy(vtkSMProxy* proxy);

  //---------------------------------------------------------------------------
  // *******  Methods for Transfer functions *********

  /**
   * Registration method for color transfer function proxies to be used after
   * PreInitializeProxy() and PostInitializeProxy() calls.
   */
  virtual bool RegisterColorTransferFunctionProxy(vtkSMProxy* proxy, const char* proxyname);
  virtual bool RegisterColorTransferFunctionProxy(vtkSMProxy* proxy)
  {
    return this->RegisterColorTransferFunctionProxy(proxy, NULL);
  }

  /**
   * Registration method for opacity transfer function proxies.
   */
  virtual bool RegisterOpacityTransferFunction(vtkSMProxy* proxy, const char* proxyname);
  virtual bool RegisterOpacityTransferFunction(vtkSMProxy* proxy)
  {
    return this->RegisterOpacityTransferFunction(proxy, NULL);
  }

  /**
   * Use this method after PreInitializeProxy() and PostInitializeProxy() to
   * register a light proxy with the proxy manager. This will also perform
   * needed python tracing.
   */
  virtual bool RegisterLightProxy(vtkSMProxy* proxy, vtkSMProxy* view, const char* proxyname);
  virtual bool RegisterLightProxy(vtkSMProxy* proxy, vtkSMProxy* view)
  {
    return this->RegisterLightProxy(proxy, view, NULL);
  }

  /**
   * Use this method after PreInitializeProxy() and PostInitializeProxy() to
   * register a texture proxy with the proxy manager. This will also perform
   * needed python tracing.
   */
  virtual bool RegisterTextureProxy(vtkSMProxy* proxy, const char* filename);

  //---------------------------------------------------------------------------
  // *******  Methods for Extractors  *********

  /**
   * Registers an extractor.
   */
  virtual bool RegisterExtractorProxy(vtkSMProxy* proxy, const char* proxyname);
  virtual bool RegisterExtractorProxy(vtkSMProxy* proxy)
  {
    return this->RegisterExtractorProxy(proxy, nullptr);
  }

  //---------------------------------------------------------------------------
  // *******  Methods for Animation   *********

  /**
   * Returns the animation scene, if any. Returns NULL if none exists.
   */
  virtual vtkSMProxy* FindAnimationScene(vtkSMSession* session);

  /**
   * Returns the animation scene for the session. If none exists, a new one will
   * be created. This may returns NULL if animation scene proxy is not available
   * in the session.
   */
  virtual vtkSMProxy* GetAnimationScene(vtkSMSession* session);

  /**
   * Return the animation track for time, if any. Returns NULL if none exists.
   */
  virtual vtkSMProxy* FindTimeAnimationTrack(vtkSMProxy* scene);

  /**
   * Return the animation track for time. If none exists, a new one will be
   * created. Returns NULL if the proxy is not available in the session.
   */
  virtual vtkSMProxy* GetTimeAnimationTrack(vtkSMProxy* scene);

  /**
   * Use this method after PreInitializeProxy() and PostInitializeProxy() to
   * register an animation proxy with the proxy manager.
   */
  virtual bool RegisterAnimationProxy(vtkSMProxy* proxy);

  /**
   * Inverse of RegisterAnimationProxy. Also unregisters cues if proxy is scene,
   * keyframes if proxy is a cue, etc.
   * Users can use either this method or the catch-all
   * vtkSMParaViewPipelineController::UnRegisterProxy() method which
   * determines the type of the proxy and then calls the appropriate method.
   */
  virtual bool UnRegisterAnimationProxy(vtkSMProxy* proxy);

  //---------------------------------------------------------------------------
  // *******  Methods for Settings   *********
  //
  /**
   * Initializes and registers proxies in the "settings" group that
   * haven't been already. This may be called whenever a new settings
   * proxy definition becomes available, say, after loading a plugin.
   */
  virtual void UpdateSettingsProxies(vtkSMSession* session);

  //---------------------------------------------------------------------------
  // ****** Methods for cleanup/finalization/deleting ******
  //
  /**
   * A catch-all method do cleanup and unregister any proxies that were
   * registered using Register..Proxy() APIs on this class. It determines what
   * known types the "proxy" is, i.e. is it a view, or pipeline, or
   * representation etc., and then calls the appropriate UnRegister...Proxy()
   * method.
   */
  virtual bool UnRegisterProxy(vtkSMProxy* proxy);

  /**
   * For a given proxy returns the name of the group used for helper proxies.
   */
  static std::string GetHelperProxyGroupName(vtkSMProxy*);

protected:
  vtkSMParaViewPipelineController();
  ~vtkSMParaViewPipelineController() override;

  /**
   * Find proxy of the group type (xmlgroup, xmltype) registered under a
   * particular group (reggroup). Returns the first proxy found, if any.
   */
  vtkSMProxy* FindProxy(
    vtkSMSessionProxyManager* pxm, const char* reggroup, const char* xmlgroup, const char* xmltype);

  //@{
  /**
   * Creates new proxies for proxies referred in vtkSMProxyListDomain for any of
   * the properties for the given proxy.
   */
  virtual bool CreateProxiesForProxyListDomains(vtkSMProxy* proxy);
  virtual void RegisterProxiesForProxyListDomains(vtkSMProxy* proxy);
  //@}

  /**
   * Setup global properties links based on hints for properties in the XML.
   */
  virtual bool SetupGlobalPropertiesLinks(vtkSMProxy* proxy);

  /**
   * To help animation representation properties such as visibility, opacity, we
   * create animation helpers.
   */
  virtual bool CreateAnimationHelpers(vtkSMProxy* proxy);

  /**
   * Unregisters know proxy dependencies that must be removed when the proxy is
   * to be deleted e.g animation cues, representations, etc.
   */
  virtual bool UnRegisterDependencies(vtkSMProxy* proxy);

  /**
   * Returns the initialization timestamp for the proxy, if available. Useful
   * for subclasses to determine which properties were modified since
   * initialization.
   */
  vtkMTimeType GetInitializationTime(vtkSMProxy*);

  //@{
  /**
   * Proxies can specify custom initialization using XML hints. This method
   * calls those initialization helpers, if any.
   */
  void ProcessInitializationHelper(vtkSMProxy*, vtkMTimeType initializationTimeStamp);
  //@}

  /**
   * If the proxy has initialization helpers, let those classes also handle
   * registration tasks, if any. This is called in `RegisterProxy`.
   */
  void ProcessInitializationHelperRegistration(vtkSMProxy*);

  /**
   * An entry point to load a catalog of OSPRay rendering materials.
   */
  virtual void DoMaterialSetup(vtkSMProxy* proxy);

private:
  vtkSMParaViewPipelineController(const vtkSMParaViewPipelineController&) = delete;
  void operator=(const vtkSMParaViewPipelineController&) = delete;
  //@}

  class vtkInternals;
  vtkInternals* Internals;
};

#endif
