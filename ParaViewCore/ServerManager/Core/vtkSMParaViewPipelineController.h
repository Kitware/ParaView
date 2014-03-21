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
// .NAME vtkSMParaViewPipelineController
// .SECTION Description
//

#ifndef __vtkSMParaViewPipelineController_h
#define __vtkSMParaViewPipelineController_h

#include "vtkSMObject.h"

class vtkSMProxy;
class vtkSMSession;
class vtkSMSessionProxyManager;

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMParaViewPipelineController : public vtkSMObject
{
public:
  static vtkSMParaViewPipelineController* New();
  vtkTypeMacro(vtkSMParaViewPipelineController, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Call this method to setup a branch new session with state considered
  // essential for ParaView session. Returns true on success.
  virtual bool InitializeSession(vtkSMSession* session);

  // Description:
  // Returns the TimeKeeper proxy associated with the session.
  virtual vtkSMProxy* FindTimeKeeper(vtkSMSession* session);

  //---------------------------------------------------------------------------
  // ******* Methods for Pipeline objects like sources/filters/readers ********

  // Description:
  // Initialize the property values using application settings.
  virtual bool PreInitializePipelineProxy(vtkSMProxy* proxy);

  // Description:
  // Safely update property values for properties whose domain may have changed
  // during the initialization process. If the property's value was manually
  // changed by the user during the initialization process, it won't be changed
  // based on the domain, even if the domain was updated.
  // This method will also register the proxy in appropriate group.
  virtual bool PostInitializePipelineProxy(vtkSMProxy* proxy);

  //---------------------------------------------------------------------------
  // *******  Methods for Views/Displays *********

  // Description:
  // Creates a new view of the given type and calls InitializeView() on it.
  virtual vtkSMProxy* CreateView(
    vtkSMSession* session, const char* xmlgroup, const char* xmltype);

  // Description:
  // Initialize a new view of the given type. Will register the newly created view
  // with the animation scene and timekeeper.
  virtual bool InitializeView(vtkSMProxy* proxy);

  // Description:
  // Pre/Post initialize a new representation proxy.
  virtual bool PreInitializeRepresentation(vtkSMProxy* proxy);
  virtual bool PostInitializeRepresentation(vtkSMProxy* proxy);

  // Description:
  // Use these methods to add/remove or show/hide representations in a view
  // (instead of directly updating the properties). This makes it possible to
  // add application logic around those actions.
  virtual bool AddRepresentationToView(vtkSMProxy* view, vtkSMProxy* repr);
  virtual bool RemoveRepresentationFromView(vtkSMProxy* view, vtkSMProxy* repr);
  virtual bool Show(vtkSMProxy* view, vtkSMProxy* repr);
  virtual bool Hide(vtkSMProxy* view, vtkSMProxy* repr);

  //---------------------------------------------------------------------------
  // *******  Methods for Animation   *********

  // Description:
  // Returns the animation scene, if any. Returns NULL if none exists.
  virtual vtkSMProxy* FindAnimationScene(vtkSMSession* session);

  // Description:
  // Returns the animation scene for the session. If none exists, a new one will
  // be created. This may returns NULL if animation scene proxy is not available
  // in the session.
  virtual vtkSMProxy* GetAnimationScene(vtkSMSession* session);

  // Description:
  // Return the animation track for time, if any. Returns NULL if none exists.
  virtual vtkSMProxy* FindTimeAnimationTrack(vtkSMProxy* scene);

  // Description:
  // Return the animation track for time. If none exists, a new one will be
  // created. Returns NULL if the proxy is not available in the session.
  virtual vtkSMProxy* GetTimeAnimationTrack(vtkSMProxy* scene);

  //---------------------------------------------------------------------------

  //---------------------------------------------------------------------------
  // ****** Methods for arbitrary proxies ******

  // Description:
  // These methods don't register/unregister the \c proxy. They perform
  // standard initialization/finaization tasks common for all types of proxies
  // in a ParaView application, except the actual registration/unregistration of
  // the \c proxy, since which group a proxy is registered to depends on its
  // type. Note these methods may register other proxies that are created as
  // part of the initialization process e.g. proxies for proxy list domains,
  // etc.
  virtual bool PreInitializeProxy(vtkSMProxy* proxy);
  virtual bool PostInitializeProxy(vtkSMProxy* proxy);
  virtual bool FinalizeProxy(vtkSMProxy*);

  // Description:
  // Convenience method, simply class PreInitializeProxy() and
  // PostInitializeProxy().
  bool InitializeProxy(vtkSMProxy* proxy)
    {
    return this->PreInitializeProxy(proxy) && this->PostInitializeProxy(proxy);
    }

  //---------------------------------------------------------------------------
  // ****** Methods for cleanup/finalization/deleting ******
  //
  // Description:
  // Method to finalize a proxy. This is same as requesting the proxy be
  // deleted. Based on the type of the proxy, this may result in other proxies
  // be finalized as well, e.g.
  // \li for a pipeline-proxy, this will finalize all representations that use
  // this proxy;
  // \li for a view-proxy, this will finalize all representations shown in that
  // view;
  // \li for an animation-scene, this will finalize all animation cues known
  // to the scene.
  virtual bool Finalize(vtkSMProxy*);

  // Description:
  // These methods are provided for completeness. Finalize() calls the
  // appropriate method based on the type of proxy pass in.
  virtual bool FinalizePipelineProxy(vtkSMProxy* proxy);
  virtual bool FinalizeRepresentation(vtkSMProxy* proxy);
  virtual bool FinalizeView(vtkSMProxy* proxy);
  virtual bool FinalizeAnimationProxy(vtkSMProxy* proxy);

  // Description:
  // Resets the session to its initial state by cleaning all pipeline
  // proxies and other non-essential proxies.
  virtual bool ResetSession(vtkSMSession* session);

  // Description:
  // For a given proxy returns the name of the group used for helper proxies.
  static vtkStdString GetHelperProxyGroupName(vtkSMProxy*);

//BTX
protected:
  vtkSMParaViewPipelineController();
  ~vtkSMParaViewPipelineController();

  // Description:
  // Find proxy of the group type (xmlgroup, xmltype) registered under a
  // particular group (reggroup). Returns the first proxy found, if any.
  vtkSMProxy* FindProxy(vtkSMSessionProxyManager* pxm,
    const char* reggroup, const char* xmlgroup, const char* xmltype);

  // Description:
  // Creates new proxies for proxies referred in vtkSMProxyListDomain for any of
  // the properties for the given proxy.
  virtual bool CreateProxiesForProxyListDomains(vtkSMProxy* proxy);
  virtual void RegisterProxiesForProxyListDomains(vtkSMProxy* proxy);

  // Description:
  // To help animation representation properties such as visibility, opacity, we
  // create animation helpers.
  virtual bool CreateAnimationHelpers(vtkSMProxy* proxy);

  virtual bool PreInitializeProxyInternal(vtkSMProxy*);
  virtual bool PostInitializeProxyInternal(vtkSMProxy*);
  virtual bool FinalizeProxyInternal(vtkSMProxy*);

  // Description:
  // Proxies in proxy-list domains can have hints that are used to setup
  // property-links to ensure that those proxies get appropriate domains.
  virtual void ProcessProxyListProxyHints(vtkSMProxy* parent, vtkSMProxy* proxyFromDomain);

private:
  vtkSMParaViewPipelineController(const vtkSMParaViewPipelineController&); // Not implemented
  void operator=(const vtkSMParaViewPipelineController&); // Not implemented

  class vtkInternals;
  vtkInternals* Internals;
//ETX
};

#endif
