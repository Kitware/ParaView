// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVKeyFrameAnimationCueForProxies
 *
 * vtkPVKeyFrameAnimationCueForProxies extends vtkPVKeyFrameAnimationCue to
 * update properties on proxies in SetAnimationValue().
 */

#ifndef vtkPVKeyFrameAnimationCueForProxies_h
#define vtkPVKeyFrameAnimationCueForProxies_h

#include "vtkPVKeyFrameAnimationCue.h"

class vtkSMProxy;
class vtkSMProperty;
class vtkSMDomain;

class VTKREMOTINGANIMATION_EXPORT vtkPVKeyFrameAnimationCueForProxies
  : public vtkPVKeyFrameAnimationCue
{
public:
  static vtkPVKeyFrameAnimationCueForProxies* New();
  vtkTypeMacro(vtkPVKeyFrameAnimationCueForProxies, vtkPVKeyFrameAnimationCue);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get the animated proxy.
   */
  void SetAnimatedProxy(vtkSMProxy*);
  vtkGetObjectMacro(AnimatedProxy, vtkSMProxy);
  ///@}

  void RemoveAnimatedProxy();

  ///@{
  /**
   * Set/Get the animated property name.
   */
  vtkSetStringMacro(AnimatedPropertyName);
  vtkGetStringMacro(AnimatedPropertyName);
  ///@}

  ///@{
  /**
   * Set/Get the animated domain name.
   */
  vtkSetStringMacro(AnimatedDomainName);
  vtkGetStringMacro(AnimatedDomainName);
  ///@}

  ///@{
  /**
   * Used to update the animated item. This API makes it possible for vtk-level
   * classes to update properties without actually linking with the
   * ServerManager library. This only works since they object are created only
   * on the client.
   */
  void BeginUpdateAnimationValues() override;
  void SetAnimationValue(int index, double value) override;
  void EndUpdateAnimationValues() override;
  ///@}

protected:
  vtkPVKeyFrameAnimationCueForProxies();
  ~vtkPVKeyFrameAnimationCueForProxies() override;

  /**
   * Get the property being animated.
   */
  vtkSMProperty* GetAnimatedProperty();

  /**
   * Get the domain being animated.
   */
  vtkSMDomain* GetAnimatedDomain();

  vtkSMProxy* AnimatedProxy;
  char* AnimatedPropertyName;
  char* AnimatedDomainName;
  int ValueIndexMax;

private:
  vtkPVKeyFrameAnimationCueForProxies(const vtkPVKeyFrameAnimationCueForProxies&) = delete;
  void operator=(const vtkPVKeyFrameAnimationCueForProxies&) = delete;
};

#endif
