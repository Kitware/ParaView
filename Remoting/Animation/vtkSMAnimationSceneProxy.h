/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMAnimationSceneProxy
 *
 * vtkSMAnimationSceneProxy observe vtkCommand::ModifiedEvent on the
 * client-side VTK-object to call UpdatePropertyInformation() every time that
 * happens.
*/

#ifndef vtkSMAnimationSceneProxy_h
#define vtkSMAnimationSceneProxy_h

#include "vtkRemotingAnimationModule.h" //needed for exports
#include "vtkSMProxy.h"

class VTKREMOTINGANIMATION_EXPORT vtkSMAnimationSceneProxy : public vtkSMProxy
{
public:
  static vtkSMAnimationSceneProxy* New();
  vtkTypeMacro(vtkSMAnimationSceneProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Setups the animation scene's playback mode and time-ranges based on the
   * timesteps available on the time-keeper proxy set on the animation scene.
   */
  virtual bool UpdateAnimationUsingDataTimeSteps();
  static bool UpdateAnimationUsingDataTimeSteps(vtkSMProxy* scene)
  {
    vtkSMAnimationSceneProxy* self = vtkSMAnimationSceneProxy::SafeDownCast(scene);
    return self ? self->UpdateAnimationUsingDataTimeSteps() : false;
  }
  //@}

  //@{
  /**
   * Returns the first animation cue (enabled or otherwise) that animates the
   * given property on the proxy. This will return nullptr if none such cue exists.
   */
  virtual vtkSMProxy* FindAnimationCue(vtkSMProxy* animatedProxy, const char* animatedPropertyName);
  static vtkSMProxy* FindAnimationCue(
    vtkSMProxy* scene, vtkSMProxy* animatedProxy, const char* animatedPropertyName)
  {
    vtkSMAnimationSceneProxy* self = vtkSMAnimationSceneProxy::SafeDownCast(scene);
    return self ? self->FindAnimationCue(animatedProxy, animatedPropertyName) : nullptr;
  }
  //@}

protected:
  vtkSMAnimationSceneProxy();
  ~vtkSMAnimationSceneProxy() override;

  /**
   * Overridden to prune start/end time properties if not applicable to the
   * state being loaded.
   */
  int LoadXMLState(vtkPVXMLElement* element, vtkSMProxyLocator* locator) override;

  /**
   * Given a class name (by setting VTKClassName) and server ids (by
   * setting ServerIDs), this methods instantiates the objects on the
   * server(s)
   */
  void CreateVTKObjects() override;

private:
  vtkSMAnimationSceneProxy(const vtkSMAnimationSceneProxy&) = delete;
  void operator=(const vtkSMAnimationSceneProxy&) = delete;

  // Called when vtkSMAnimationScene::UpdateStartEndTimesEvent is fired.
  void OnUpdateStartEndTimesEvent(vtkObject*, unsigned long, void*);
};

#endif
