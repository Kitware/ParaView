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
// .NAME vtkPVKeyFrameAnimationCueForProxies
// .SECTION Description
// vtkPVKeyFrameAnimationCueForProxies extends vtkPVKeyFrameAnimationCue to
// update properties on proxies in SetAnimationValue().

#ifndef __vtkPVKeyFrameAnimationCueForProxies_h
#define __vtkPVKeyFrameAnimationCueForProxies_h

#include "vtkPVKeyFrameAnimationCue.h"

class vtkSMProxy;
class vtkSMProperty;
class vtkSMDomain;

class VTK_EXPORT vtkPVKeyFrameAnimationCueForProxies : public vtkPVKeyFrameAnimationCue
{
public:
  static vtkPVKeyFrameAnimationCueForProxies* New();
  vtkTypeMacro(vtkPVKeyFrameAnimationCueForProxies, vtkPVKeyFrameAnimationCue);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the animated proxy.
  void SetAnimatedProxy(vtkSMProxy*);
  vtkGetObjectMacro(AnimatedProxy, vtkSMProxy);

  void RemoveAnimatedProxy();

  // Description:
  // Set/Get the animated property name.
  vtkSetStringMacro(AnimatedPropertyName);
  vtkGetStringMacro(AnimatedPropertyName);

  // Description:
  // Set/Get the animated domain name.
  vtkSetStringMacro(AnimatedDomainName);
  vtkGetStringMacro(AnimatedDomainName);

  // Description:
  // Used to update the animated item. This API makes it possible for vtk-level
  // classes to update properties without actually linking with the
  // ServerManager library. This only works since they object are created only
  // on the client.
  virtual void BeginUpdateAnimationValues();
  virtual void SetAnimationValue(int index, double value);
  virtual void EndUpdateAnimationValues();

//BTX
protected:
  vtkPVKeyFrameAnimationCueForProxies();
  ~vtkPVKeyFrameAnimationCueForProxies();

  // Description:
  // Get the property being animated.
  vtkSMProperty* GetAnimatedProperty();

  // Description:
  // Get the domain being animated.
  vtkSMDomain* GetAnimatedDomain();

  vtkSMProxy* AnimatedProxy;
  char* AnimatedPropertyName;
  char* AnimatedDomainName;
  int ValueIndexMax;

private:
  vtkPVKeyFrameAnimationCueForProxies(const vtkPVKeyFrameAnimationCueForProxies&); // Not implemented
  void operator=(const vtkPVKeyFrameAnimationCueForProxies&); // Not implemented
//ETX
};

#endif
