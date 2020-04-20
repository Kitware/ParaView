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
#include "vtkSMProxy.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSessionProxyManager.h"

#include <QList>
#include <QtDebug>

#include "pqSMAdaptor.h"
#include "pqServer.h"

//-----------------------------------------------------------------------------
pqAnimationCue::pqAnimationCue(const QString& group, const QString& name, vtkSMProxy* proxy,
  pqServer* server, QObject* _parent /*=NULL*/)
  : pqProxy(group, name, proxy, server, _parent)
{
  this->KeyFrameType = "CompositeKeyFrame";

  vtkEventQtSlotConnect* connector = this->getConnector();
  if (proxy->GetProperty("AnimatedProxy"))
  {
    connector->Connect(
      proxy->GetProperty("AnimatedProxy"), vtkCommand::ModifiedEvent, this, SIGNAL(modified()));
  }
  if (proxy->GetProperty("AnimatedPropertyName"))
  {
    // since some cue like that for Camera doesn't have this property.
    connector->Connect(proxy->GetProperty("AnimatedPropertyName"), vtkCommand::ModifiedEvent, this,
      SIGNAL(modified()));
  }

  if (proxy->GetProperty("AnimatedElement"))
  {
    connector->Connect(
      proxy->GetProperty("AnimatedElement"), vtkCommand::ModifiedEvent, this, SIGNAL(modified()));
  }

  connector->Connect(
    proxy->GetProperty("Enabled"), vtkCommand::ModifiedEvent, this, SLOT(onEnabledModified()));

  connector->Connect(proxy, vtkCommand::ModifiedEvent, this, SIGNAL(keyframesModified()));
}

//-----------------------------------------------------------------------------
pqAnimationCue::~pqAnimationCue()
{
}

//-----------------------------------------------------------------------------
void pqAnimationCue::addKeyFrameInternal(vtkSMProxy* keyframe)
{
  this->proxyManager()->RegisterProxy("animation",
    QString("KeyFrame%1").arg(keyframe->GetGlobalIDAsString()).toLocal8Bit().data(), keyframe);
}

//-----------------------------------------------------------------------------
void pqAnimationCue::removeKeyFrameInternal(vtkSMProxy* keyframe)
{
  vtkSMSessionProxyManager* pxm = this->proxyManager();
  pxm->UnRegisterProxy("animation", pxm->GetProxyName("animation", keyframe), keyframe);
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqAnimationCue::getAnimatedProxy() const
{
  vtkSMProxy* proxy = pqSMAdaptor::getProxyProperty(this->getProxy()->GetProperty("AnimatedProxy"));
  return proxy;
}

//-----------------------------------------------------------------------------
vtkSMProperty* pqAnimationCue::getAnimatedProperty() const
{
  vtkSMProxy* selfProxy = this->getProxy();
  vtkSMProxy* proxy = pqSMAdaptor::getProxyProperty(selfProxy->GetProperty("AnimatedProxy"));
  if (proxy && selfProxy->GetProperty("AnimatedPropertyName"))
  {
    QString pname =
      pqSMAdaptor::getElementProperty(selfProxy->GetProperty("AnimatedPropertyName")).toString();
    if (pname != "")
    {
      return proxy->GetProperty(pname.toLocal8Bit().data());
    }
  }

  return 0;
}

//-----------------------------------------------------------------------------
int pqAnimationCue::getAnimatedPropertyIndex() const
{
  return pqSMAdaptor::getElementProperty(this->getProxy()->GetProperty("AnimatedElement")).toInt();
}

//-----------------------------------------------------------------------------
void pqAnimationCue::setEnabled(bool enable)
{
  pqSMAdaptor::setElementProperty(this->getProxy()->GetProperty("Enabled"), enable ? 1 : 0);
  this->getProxy()->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
bool pqAnimationCue::isEnabled() const
{
  return pqSMAdaptor::getElementProperty(this->getProxy()->GetProperty("Enabled")).toBool();
}

//-----------------------------------------------------------------------------
void pqAnimationCue::onEnabledModified()
{
  Q_EMIT this->enabled(this->isEnabled());
}

//-----------------------------------------------------------------------------
int pqAnimationCue::getNumberOfKeyFrames() const
{
  vtkSMProxyProperty* pp =
    vtkSMProxyProperty::SafeDownCast(this->getProxy()->GetProperty("KeyFrames"));
  return (pp ? pp->GetNumberOfProxies() : 0);
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqAnimationCue::getKeyFrame(int index) const
{
  vtkSMProxyProperty* pp =
    vtkSMProxyProperty::SafeDownCast(this->getProxy()->GetProperty("KeyFrames"));
  if (pp && index >= 0 && (int)(pp->GetNumberOfProxies()) > index)
  {
    return pp->GetProxy(index);
  }
  return NULL;
}

//-----------------------------------------------------------------------------
QList<vtkSMProxy*> pqAnimationCue::getKeyFrames() const
{
  QList<vtkSMProxy*> list;
  vtkSMProxyProperty* pp =
    vtkSMProxyProperty::SafeDownCast(this->getProxy()->GetProperty("KeyFrames"));
  for (unsigned int cc = 0; pp && cc < pp->GetNumberOfProxies(); cc++)
  {
    list.push_back(pp->GetProxy(cc));
  }
  return list;
}

//-----------------------------------------------------------------------------
void pqAnimationCue::deleteKeyFrame(int index)
{
  QList<vtkSMProxy*> keyframes = this->getKeyFrames();
  if (index < 0 || index >= keyframes.size())
  {
    qDebug() << "Invalid index " << index;
    return;
  }

  vtkSMProxy* keyframe = keyframes[index];
  keyframes.removeAt(index);

  std::vector<vtkSMProxy*> proxy_vector;
  foreach (vtkSMProxy* curKf, keyframes)
  {
    proxy_vector.push_back(curKf);
  }

  vtkSMProxyProperty* pp =
    vtkSMProxyProperty::SafeDownCast(this->getProxy()->GetProperty("KeyFrames"));
  pp->SetProxies(static_cast<unsigned int>(proxy_vector.size()),
    (proxy_vector.size() > 0 ? &proxy_vector[0] : NULL));
  this->getProxy()->UpdateVTKObjects();
  this->removeKeyFrameInternal(keyframe);
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqAnimationCue::insertKeyFrame(int index)
{
  vtkSMSessionProxyManager* pxm = this->proxyManager();

  // Get the current keyframes.
  QList<vtkSMProxy*> keyframes = this->getKeyFrames();

  vtkSMProxy* kf = pxm->NewProxy("animation_keyframes", this->KeyFrameType.toLocal8Bit().data());
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
    if (keyframes.size() > 2)
    {
      double oldtime =
        pqSMAdaptor::getElementProperty(keyframes[1]->GetProperty("KeyTime")).toDouble();
      double nexttime =
        pqSMAdaptor::getElementProperty(keyframes[2]->GetProperty("KeyTime")).toDouble();
      if (oldtime == 0.0)
      {
        oldtime = (nexttime + oldtime) / 2.0;
        pqSMAdaptor::setElementProperty(keyframes[1]->GetProperty("KeyTime"), oldtime);
        keyframes[1]->UpdateVTKObjects();
      }
    }
    else if (keyframes.size() == 2)
    {
      double oldtime =
        pqSMAdaptor::getElementProperty(keyframes[1]->GetProperty("KeyTime")).toDouble();
      if (oldtime == 0.0)
      {
        pqSMAdaptor::setElementProperty(keyframes[1]->GetProperty("KeyTime"), 0.5);
        keyframes[1]->UpdateVTKObjects();
      }
    }
  }
  else if (index == keyframes.size() - 1)
  {
    keyTime = 1.0;
    // If another keyframe exists with keytime 1 as the previous last key frame
    // with key time 1.0, we change its keytime to be between 1.0 and the
    // keytime for the keyframe before it, if one exists or 0.5.
    double prev_time =
      pqSMAdaptor::getElementProperty(keyframes[index - 1]->GetProperty("KeyTime")).toDouble();
    if (index >= 2 && prev_time == 1.0)
    {
      double prev_2_time =
        pqSMAdaptor::getElementProperty(keyframes[index - 2]->GetProperty("KeyTime")).toDouble();
      pqSMAdaptor::setElementProperty(
        keyframes[index - 1]->GetProperty("KeyTime"), (prev_2_time + prev_time) / 2.0);
      keyframes[index - 1]->UpdateVTKObjects();
    }
    else if (prev_time == 1.0)
    {
      pqSMAdaptor::setElementProperty(keyframes[index - 1]->GetProperty("KeyTime"), 0.5);
      keyframes[index - 1]->UpdateVTKObjects();
    }
  }
  else
  {
    double prev_time =
      pqSMAdaptor::getElementProperty(keyframes[index - 1]->GetProperty("KeyTime")).toDouble();
    double next_time =
      pqSMAdaptor::getElementProperty(keyframes[index + 1]->GetProperty("KeyTime")).toDouble();
    keyTime = (prev_time + next_time) / 2.0;
  }

  // Register the proxy
  kf->UpdateVTKObjects();
  this->addKeyFrameInternal(kf);

  // Set the KeyTime property
  pqSMAdaptor::setElementProperty(kf->GetProperty("KeyTime"), keyTime);
  kf->UpdateVTKObjects();

  std::vector<vtkSMProxy*> proxy_vector;
  foreach (vtkSMProxy* curKf, keyframes)
  {
    proxy_vector.push_back(curKf);
  }

  vtkSMProxyProperty* pp =
    vtkSMProxyProperty::SafeDownCast(this->getProxy()->GetProperty("KeyFrames"));
  pp->SetProxies(static_cast<unsigned int>(proxy_vector.size()),
    (proxy_vector.size() > 0 ? &proxy_vector[0] : NULL));
  this->getProxy()->UpdateVTKObjects();

  kf->Delete();
  return kf;
}
