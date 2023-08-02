// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqAnimationCue.h"

#include "vtkEventQtSlotConnect.h"
#include "vtkPVCameraCueManipulator.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMTrace.h"

#include <QCoreApplication>
#include <QList>
#include <QtDebug>

#include "pqApplicationCore.h"
#include "pqSMAdaptor.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"

//-----------------------------------------------------------------------------
pqAnimationCue::pqAnimationCue(const QString& group, const QString& name, vtkSMProxy* proxy,
  pqServer* server, QObject* _parent /*=nullptr*/)
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

  // keep the GUI updated when the client side vtkPVAnimationCue is modified
  connector->Connect(vtkObject::SafeDownCast(proxy->GetClientSideObject()),
    vtkCommand::ModifiedEvent, this, SIGNAL(keyframesModified()));
}

//-----------------------------------------------------------------------------
pqAnimationCue::~pqAnimationCue() = default;

//-----------------------------------------------------------------------------
void pqAnimationCue::addKeyFrameInternal(vtkSMProxy* keyframe)
{
  this->proxyManager()->RegisterProxy("animation",
    QString("KeyFrame%1").arg(keyframe->GetGlobalIDAsString()).toUtf8().data(), keyframe);
}

//-----------------------------------------------------------------------------
void pqAnimationCue::removeKeyFrameInternal(vtkSMProxy* keyframe)
{
  SM_SCOPED_TRACE(Delete).arg(keyframe);
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
QString pqAnimationCue::getCameraModeName(int mode)
{
  switch (mode)
  {
    case vtkPVCameraCueManipulator::PATH:
      return QString("path");
    case vtkPVCameraCueManipulator::FOLLOW_DATA:
      return QString("data");
    case vtkPVCameraCueManipulator::CAMERA:
      return QString("camera");
    default:
      break;
  }

  return QString("");
}

//-----------------------------------------------------------------------------
QString pqAnimationCue::getAnimatedPropertyName() const
{
  vtkSMProxy* selfProxy = this->getProxy();
  if (selfProxy->GetProperty("AnimatedPropertyName"))
  {
    return pqSMAdaptor::getElementProperty(selfProxy->GetProperty("AnimatedPropertyName"))
      .toString();
  }
  else if (selfProxy->GetProperty("Mode"))
  {
    int mode = pqSMAdaptor::getElementProperty(selfProxy->GetProperty("Mode")).toInt();
    return pqAnimationCue::getCameraModeName(mode);
  }

  return QString("");
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
      return proxy->GetProperty(pname.toUtf8().data());
    }
  }

  return nullptr;
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
  this->getProxy()->UpdateVTKObjects();
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
  return nullptr;
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
  Q_FOREACH (vtkSMProxy* curKf, keyframes)
  {
    proxy_vector.push_back(curKf);
  }

  vtkSMProxyProperty* pp =
    vtkSMProxyProperty::SafeDownCast(this->getProxy()->GetProperty("KeyFrames"));
  pp->SetProxies(static_cast<unsigned int>(proxy_vector.size()),
    (!proxy_vector.empty() ? &proxy_vector[0] : nullptr));
  this->getProxy()->UpdateVTKObjects();
  this->removeKeyFrameInternal(keyframe);
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqAnimationCue::insertKeyFrame(int index)
{
  vtkSMSessionProxyManager* pxm = this->proxyManager();

  // Get the current keyframes.
  QList<vtkSMProxy*> keyframes = this->getKeyFrames();

  vtkSMProxy* kf = pxm->NewProxy("animation_keyframes", this->KeyFrameType.toUtf8().data());
  if (!kf)
  {
    qDebug() << "Could not create new proxy " << this->KeyFrameType;
    return nullptr;
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
  Q_FOREACH (vtkSMProxy* curKf, keyframes)
  {
    proxy_vector.push_back(curKf);
  }

  vtkSMProxyProperty* pp =
    vtkSMProxyProperty::SafeDownCast(this->getProxy()->GetProperty("KeyFrames"));
  pp->SetProxies(static_cast<unsigned int>(proxy_vector.size()),
    (!proxy_vector.empty() ? &proxy_vector[0] : nullptr));
  this->getProxy()->UpdateVTKObjects();

  kf->Delete();
  return kf;
}

//-----------------------------------------------------------------------------
QString pqAnimationCue::getDisplayName()
{
  if (this->isCameraCue())
  {
    vtkSMProxy* pxy = this->getAnimatedProxy();
    pqServerManagerModel* model = pqApplicationCore::instance()->getServerManagerModel();
    if (pqProxy* animation_pqproxy = model->findItem<pqProxy*>(pxy))
    {
      return QString("Camera - %1").arg(animation_pqproxy->getSMName());
    }

    return "Camera";
  }
  else if (this->isPythonCue())
  {
    return "Python";
  }
  else
  {
    pqServerManagerModel* model = pqApplicationCore::instance()->getServerManagerModel();

    vtkSMProxy* pxy = this->getAnimatedProxy();
    vtkSMProperty* pty = this->getAnimatedProperty();
    QString propertyLabel = QCoreApplication::translate(
      "ServerManagerXML", this->getAnimatedPropertyName().toUtf8().data());
    if (pqSMAdaptor::getPropertyType(pty) == pqSMAdaptor::MULTIPLE_ELEMENTS)
    {
      propertyLabel = QString("%1 (%2)").arg(propertyLabel).arg(this->getAnimatedPropertyIndex());
    }

    if (pqProxy* animation_pqproxy = model->findItem<pqProxy*>(pxy))
    {
      return QString("%1 - %2").arg(animation_pqproxy->getSMName()).arg(propertyLabel);
    }

    // could be a helper proxy
    QString helper_key;
    if (pqProxy* pqproxy = pqProxy::findProxyWithHelper(pxy, helper_key))
    {
      vtkSMProperty* prop = pqproxy->getProxy()->GetProperty(helper_key.toUtf8().data());
      if (prop)
      {
        return QString("%1 - %2 - %3")
          .arg(pqproxy->getSMName())
          .arg(QCoreApplication::translate("ServerManagerXML", prop->GetXMLLabel()))
          .arg(propertyLabel);
      }
      return QString("%1 - %2").arg(pqproxy->getSMName()).arg(propertyLabel);
    }
  }

  return QString("<%1>").arg(tr("unrecognized"));
}

//-----------------------------------------------------------------------------
bool pqAnimationCue::isCameraCue()
{
  return QString("CameraAnimationCue") == this->getProxy()->GetXMLName();
}

//-----------------------------------------------------------------------------
bool pqAnimationCue::isPythonCue()
{
  return QString("PythonAnimationCue") == this->getProxy()->GetXMLName();
}

//-----------------------------------------------------------------------------
bool pqAnimationCue::isTimekeeperCue()
{
  return QString("TimeAnimationCue") == this->getProxy()->GetXMLName();
}
