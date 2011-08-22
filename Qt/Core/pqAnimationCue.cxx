/*=========================================================================

   Program: ParaView
   Module:    pqAnimationCue.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2. 

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
#include "pqAnimationCue.h"

#include "vtkEventQtSlotConnect.h"
#include "vtkProcessModule.h"
#include "vtkSmartPointer.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"

#include <QList>
#include <QtDebug>

#include "pqSMAdaptor.h"
#include "pqServer.h"

class pqAnimationCue::pqInternals
{
public:
  vtkSmartPointer<vtkSMProxy> Manipulator;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;
  pqInternals()
    {
    this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
    }
};

//-----------------------------------------------------------------------------
pqAnimationCue::pqAnimationCue(const QString& group, const QString& name,
    vtkSMProxy* proxy, pqServer* server, QObject* _parent/*=NULL*/)
: pqProxy(group, name, proxy, server, _parent)
{
  this->ManipulatorType = "KeyFrameAnimationCueManipulator";
  this->KeyFrameType = "CompositeKeyFrame";

  this->Internal = new pqAnimationCue::pqInternals();

  if (proxy->GetProperty("Manipulator"))
    {
    this->Internal->VTKConnect->Connect(
      proxy->GetProperty("Manipulator"), vtkCommand::ModifiedEvent,
      this, SLOT(onManipulatorModified()));
    }

  if (proxy->GetProperty("AnimatedProxy"))
    {
    this->Internal->VTKConnect->Connect(
      proxy->GetProperty("AnimatedProxy"), vtkCommand::ModifiedEvent,
      this, SIGNAL(modified()));
    }
  if (proxy->GetProperty("AnimatedPropertyName"))
    {
    // since some cue like that for Camera doesn't have this property.
    this->Internal->VTKConnect->Connect(
      proxy->GetProperty("AnimatedPropertyName"), vtkCommand::ModifiedEvent,
      this, SIGNAL(modified()));
    }

  if (proxy->GetProperty("AnimatedElement"))
    {
    this->Internal->VTKConnect->Connect(
      proxy->GetProperty("AnimatedElement"), vtkCommand::ModifiedEvent,
      this, SIGNAL(modified()));
    }

  this->Internal->VTKConnect->Connect(
    proxy->GetProperty("Enabled"), vtkCommand::ModifiedEvent,
    this, SLOT(onEnabledModified()));

  this->onManipulatorModified();
}

//-----------------------------------------------------------------------------
pqAnimationCue::~pqAnimationCue()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqAnimationCue::addKeyFrameInternal(vtkSMProxy* keyframe)
{
  this->proxyManager()->RegisterProxy("animation",
    QString("KeyFrame%1").arg(keyframe->GetGlobalIDAsString()).toAscii().data(),
    keyframe);
}

//-----------------------------------------------------------------------------
void pqAnimationCue::removeKeyFrameInternal(vtkSMProxy* keyframe)
{
  vtkSMProxyManager* pxm = this->proxyManager();
  pxm->UnRegisterProxy("animation",
    pxm->GetProxyName("animation", keyframe), keyframe);
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqAnimationCue::getManipulatorProxy() const
{
  return this->Internal->Manipulator;
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqAnimationCue::getAnimatedProxy() const
{
  vtkSMProxy* proxy = pqSMAdaptor::getProxyProperty(
    this->getProxy()->GetProperty("AnimatedProxy"));
  return proxy;
}

//-----------------------------------------------------------------------------
vtkSMProperty* pqAnimationCue::getAnimatedProperty() const
{
  vtkSMProxy* proxy = pqSMAdaptor::getProxyProperty(
    this->getProxy()->GetProperty("AnimatedProxy"));
  if (proxy)
    {
    QString pname = pqSMAdaptor::getElementProperty(
      this->getProxy()->GetProperty("AnimatedPropertyName")).toString();
    if (pname != "")
      {
      return proxy->GetProperty(pname.toAscii().data());
      }
    }
  return 0;
}

//-----------------------------------------------------------------------------
int pqAnimationCue::getAnimatedPropertyIndex() const
{
  return pqSMAdaptor::getElementProperty(
    this->getProxy()->GetProperty("AnimatedElement")).toInt();
}

//-----------------------------------------------------------------------------
void pqAnimationCue::setDefaultPropertyValues()
{
  this->Superclass::setDefaultPropertyValues();

  vtkSMProxy* proxy = this->getProxy();
  if (!this->Internal->Manipulator && proxy->GetProperty("Manipulator"))
    {
    vtkSMProxyManager* pxm = this->proxyManager();
    vtkSMProxy* manip = 
      pxm->NewProxy("animation_manipulators", 
        this->ManipulatorType.toAscii().data());
    this->addHelperProxy("Manipulator", manip);
    manip->Delete();
    pqSMAdaptor::setProxyProperty(proxy->GetProperty("Manipulator"),
      manip);
    }

  // All cues are always normalized, this ensures that the
  // Cue times are valid even when the scene times are changed.
  pqSMAdaptor::setEnumerationProperty(proxy->GetProperty("TimeMode"),
    "Normalized");
  proxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void pqAnimationCue::setEnabled(bool enable)
{
  pqSMAdaptor::setElementProperty(this->getProxy()->GetProperty("Enabled"),
    enable? 1 : 0);
  this->getProxy()->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
bool pqAnimationCue::isEnabled() const
{
  return pqSMAdaptor::getElementProperty(
    this->getProxy()->GetProperty("Enabled")).toBool();
}

//-----------------------------------------------------------------------------
void pqAnimationCue::onEnabledModified()
{
  emit this->enabled(this->isEnabled());
}

//-----------------------------------------------------------------------------
void pqAnimationCue::onManipulatorModified()
{
  vtkSMProxy* myproxy = this->getProxy();
  vtkSMProxy* manip = 0;
  if (!myproxy->GetProperty("Manipulator") && myproxy->GetProperty("KeyFrames"))
    {
    // Manipulator is an internal subproxy of this cue.
    manip = myproxy;
    }
  else
    {
    manip = pqSMAdaptor::getProxyProperty(
      this->getProxy()->GetProperty("Manipulator"));
    }
  
  if (manip != this->Internal->Manipulator)
    {
    if (this->Internal->Manipulator)
      {
      this->Internal->VTKConnect->Disconnect(
        this->Internal->Manipulator, 0, this, 0);
      }

    this->Internal->Manipulator = manip;

    if (this->Internal->Manipulator)
      {
      this->Internal->VTKConnect->Connect(
        this->Internal->Manipulator, vtkCommand::ModifiedEvent,
        this, SIGNAL(keyframesModified()));
      }

    emit this->keyframesModified();
    }
}

//-----------------------------------------------------------------------------
int pqAnimationCue::getNumberOfKeyFrames() const
{
  if (this->Internal->Manipulator)
    {
    vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
      this->Internal->Manipulator->GetProperty("KeyFrames"));
    return (pp? pp->GetNumberOfProxies(): 0);
    }
  return 0;
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqAnimationCue::getKeyFrame(int index) const
{
  if (this->Internal->Manipulator)
    {
    vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
      this->Internal->Manipulator->GetProperty("KeyFrames"));
    if (pp && index >=0 && (int)(pp->GetNumberOfProxies()) > index )
      {
      return pp->GetProxy(index);
      }
    }
  return 0;
}

//-----------------------------------------------------------------------------
QList<vtkSMProxy*> pqAnimationCue::getKeyFrames() const
{
  QList<vtkSMProxy*> list;
  if (this->Internal->Manipulator)
    {
    vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
      this->Internal->Manipulator->GetProperty("KeyFrames"));
    for (unsigned int cc=0; pp && cc < pp->GetNumberOfProxies(); cc++)
      {
      list.push_back(pp->GetProxy(cc));
      }
    }
  return list;
}

//-----------------------------------------------------------------------------
void pqAnimationCue::deleteKeyFrame(int index)
{
  if (!this->Internal->Manipulator)
    {
    qDebug() << "Cue does not have a KeyFrame manipulator. "
      << "One cannot delete keyframes to this Cue.";
    return;
    }

  QList<vtkSMProxy*> keyframes = this->getKeyFrames();
  if (index <0 || index >= keyframes.size())
    {
    qDebug() << "Invalid index " << index;
    return;
    }

  vtkSMProxy* keyframe = keyframes[index];
  keyframes.removeAt(index);

  vtkSMProxyProperty* pp =vtkSMProxyProperty::SafeDownCast(
    this->Internal->Manipulator->GetProperty("KeyFrames"));
  pp->RemoveAllProxies();

  foreach(vtkSMProxy* curKf, keyframes)
    {
    pp->AddProxy(curKf);
    }
  this->Internal->Manipulator->UpdateVTKObjects();
  this->removeKeyFrameInternal(keyframe);
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqAnimationCue::insertKeyFrame(int index)
{
  if (!this->Internal->Manipulator)
    {
    qDebug() << "Cue does not have a KeyFrame manipulator. "
      << "One cannot add keyframes to this Cue.";
    return 0;
    }

  vtkSMProxyManager* pxm = this->proxyManager();

  // Get the current keyframes.
  QList<vtkSMProxy*> keyframes = this->getKeyFrames();
  
  vtkSMProxy* kf = pxm->NewProxy("animation_keyframes", 
    this->KeyFrameType.toAscii().data());
  if (!kf)
    {
    qDebug() << "Could not create new proxy " << this->KeyFrameType;
    return 0;
    }

  keyframes.insert(index, kf);
  double keyTime;
  if (index == 0)
    {
    keyTime = 0.0;

    // If another keyframe existed at index 0 with keytime 0, we change its
    // keytime to be between 0 and the keytime for the key frame after it (if
    // any exists) or 0.5.
    if (keyframes.size()>2)
      {
      double oldtime = pqSMAdaptor::getElementProperty(
        keyframes[1]->GetProperty("KeyTime")).toDouble();
      double nexttime = pqSMAdaptor::getElementProperty(
        keyframes[2]->GetProperty("KeyTime")).toDouble();
      if (oldtime == 0.0)
        {
        oldtime = (nexttime+oldtime)/2.0;
        pqSMAdaptor::setElementProperty(keyframes[1]->GetProperty("KeyTime"),
          oldtime);
        keyframes[1]->UpdateVTKObjects();
        }
      }
    else if (keyframes.size()==2)
      {
      double oldtime = pqSMAdaptor::getElementProperty(
        keyframes[1]->GetProperty("KeyTime")).toDouble();
      if (oldtime == 0.0)
        {
        pqSMAdaptor::setElementProperty(keyframes[1]->GetProperty("KeyTime"),
          0.5);
        keyframes[1]->UpdateVTKObjects();
        }
      }
    }
  else if (index == keyframes.size()-1)
    {
    keyTime = 1.0;
    // If another keyframe exists with keytime 1 as the previous last key frame
    // with key time 1.0, we change its keytime to be between 1.0 and the
    // keytime for the keyframe before it, if one exists or 0.5.
    double prev_time = pqSMAdaptor::getElementProperty(
      keyframes[index-1]->GetProperty("KeyTime")).toDouble();
    if (index >= 2 && prev_time==1.0)
      {
      double prev_2_time = pqSMAdaptor::getElementProperty(
        keyframes[index-2]->GetProperty("KeyTime")).toDouble();
      pqSMAdaptor::setElementProperty(keyframes[index-1]->GetProperty("KeyTime"),
        (prev_2_time + prev_time)/2.0);
      keyframes[index-1]->UpdateVTKObjects();
      }
    else if (prev_time==1.0)
      {
      pqSMAdaptor::setElementProperty(keyframes[index-1]->GetProperty("KeyTime"),
        0.5);
      keyframes[index-1]->UpdateVTKObjects();
      }
    }
  else 
    {
    double prev_time = pqSMAdaptor::getElementProperty(
      keyframes[index-1]->GetProperty("KeyTime")).toDouble();
    double next_time = pqSMAdaptor::getElementProperty(
      keyframes[index+1]->GetProperty("KeyTime")).toDouble();
    keyTime = (prev_time + next_time)/2.0;
    }

  // Register the proxy
  kf->UpdateVTKObjects();
  this->addKeyFrameInternal(kf);

  // Set the KeyTime property
  pqSMAdaptor::setElementProperty(kf->GetProperty("KeyTime"), keyTime);
  kf->UpdateVTKObjects();

  vtkSMProxyProperty* pp =vtkSMProxyProperty::SafeDownCast(
    this->Internal->Manipulator->GetProperty("KeyFrames"));
  pp->RemoveAllProxies();

  foreach(vtkSMProxy* curKf, keyframes)
    {
    pp->AddProxy(curKf);
    }
  this->Internal->Manipulator->UpdateVTKObjects();

  kf->Delete();
  return kf;
}
