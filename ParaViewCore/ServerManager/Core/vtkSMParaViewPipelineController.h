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
  virtual bool PreInitializeProxy(vtkSMProxy* proxy);
  virtual bool PostInitializeProxy(vtkSMProxy* proxy);
  // Description:
  // Convenience method, simply class PreInitializeProxy() and
  // PostInitializeProxy().
  bool InitializeProxy(vtkSMProxy* proxy)
    {
    return this->PreInitializeProxy(proxy) && this->PostInitializeProxy(proxy);
    }

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

private:
  vtkSMParaViewPipelineController(const vtkSMParaViewPipelineController&); // Not implemented
  void operator=(const vtkSMParaViewPipelineController&); // Not implemented

  class vtkInternals;
  vtkInternals* Internals;
//ETX
};

#endif
