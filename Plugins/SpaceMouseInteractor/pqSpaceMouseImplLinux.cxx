// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqSpaceMouseImplLinux.h"

#include "pqRenderView.h"
#include "pqServer.h"
#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkPVRenderView.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkTransform.h"

#include <QSocketNotifier>

#include <spnav.h>

#include <algorithm>
#include <cerrno>
#include <sys/socket.h>

namespace
{
// libspnav reports roughly 350 units at full deflection and motion periods in milliseconds.
constexpr double AxisRange = 350.0;
constexpr double TranslationSpeed = 1.0; // camera distances per second
constexpr double RotationSpeed = 90.0;   // degrees per second
constexpr unsigned int MaxFrameDurationMs = 50;
constexpr int BusyRetryIntervalMs = 50;
}

pqSpaceMouseImpl::pqSpaceMouseImpl()
{
  this->ReconnectTimer.setInterval(2000);
  QObject::connect(
    &this->ReconnectTimer, &pqTimer::timeout, this, [this]() { this->connectToDaemon(); });

  this->FrameTimer.setSingleShot(true);
  this->FrameTimer.setInterval(16);
  QObject::connect(&this->FrameTimer, &pqTimer::timeout, this, [this]() { this->flushMotion(); });

  this->IdleTimer.setSingleShot(true);
  this->IdleTimer.setInterval(100);
  QObject::connect(&this->IdleTimer, &pqTimer::timeout, this,
    [this]()
    {
      this->FrameTimer.stop();
      this->flushMotion();
      this->endInteraction();
    });

  this->EndTimer.setInterval(BusyRetryIntervalMs);
  QObject::connect(&this->EndTimer, &pqTimer::timeout, this, [this]() { this->endInteraction(); });
}

pqSpaceMouseImpl::~pqSpaceMouseImpl()
{
  this->setActiveView(nullptr);
}

void pqSpaceMouseImpl::setActiveView(pqView* view)
{
  auto* newView = qobject_cast<pqRenderView*>(view);
  if (this->View != newView)
  {
    this->IdleTimer.stop();
    this->clearMotion();
    this->endInteraction();
    this->View = newView;
  }

  if (this->View)
  {
    if (!this->Notifier && !this->ReconnectTimer.isActive())
    {
      this->connectToDaemon();
    }
    else if (this->Notifier)
    {
      spnav_remove_events(SPNAV_EVENT_MOTION);
    }
  }
  else
  {
    this->ReconnectTimer.stop();
    this->disconnectFromDaemon();
  }
}

void pqSpaceMouseImpl::connectToDaemon()
{
  if (!this->View || this->Notifier)
  {
    return;
  }

  if (spnav_open() == -1)
  {
    this->ReconnectTimer.start();
    return;
  }
  if (spnav_fd() < 0)
  {
    spnav_close();
    this->ReconnectTimer.start();
    return;
  }

  this->ReconnectTimer.stop();
  this->Notifier = new QSocketNotifier(spnav_fd(), QSocketNotifier::Read, this);
  QObject::connect(
    this->Notifier, &QSocketNotifier::activated, this, [this]() { this->readEvents(); });
}

void pqSpaceMouseImpl::disconnectFromDaemon()
{
  this->clearMotion();
  delete this->Notifier;
  this->Notifier = nullptr;
  spnav_close();
}

void pqSpaceMouseImpl::readEvents()
{
  if (!this->View)
  {
    this->disconnectFromDaemon();
    return;
  }
  spnav_event event;
  while (this->Notifier && spnav_poll_event(&event))
  {
    if (event.type == SPNAV_EVENT_MOTION)
    {
      const auto& motion = event.motion;
      // Discard input while the current or previous view is busy. Do not replay
      // motion received during a pipeline update after it completes.
      if ((this->Interacting &&
            (this->InteractionView != this->View || this->EndTimer.isActive()) &&
            !this->endInteraction()) ||
        this->isServerBusy(this->View))
      {
        this->IdleTimer.stop();
        this->clearMotion();
        this->endInteraction();
        continue;
      }
      if (motion.x || motion.y || motion.z || motion.rx || motion.ry || motion.rz)
      {
        // Keep the latest device state and cap the accumulated period so a delayed
        // GUI frame does not cause the camera to jump.
        this->PendingMotion = { { static_cast<double>(motion.x), static_cast<double>(motion.y),
          static_cast<double>(motion.z), static_cast<double>(motion.rx),
          static_cast<double>(motion.ry), static_cast<double>(motion.rz) } };
        this->PendingDurationMs = std::min(MaxFrameDurationMs,
          this->PendingDurationMs +
            std::min(motion.period ? motion.period : 16u, MaxFrameDurationMs));
        this->HasPendingMotion = true;
        this->StopAfterFrame = false;
        this->IdleTimer.start();
        if (!this->FrameTimer.isActive())
        {
          this->FrameTimer.start();
        }
      }
      else
      {
        this->IdleTimer.stop();
        if (this->HasPendingMotion)
        {
          this->StopAfterFrame = true;
          this->FrameTimer.start(0);
        }
        else
        {
          this->endInteraction();
        }
      }
    }
    else if (event.type == SPNAV_EVENT_DEV && event.dev.op == SPNAV_DEV_RM)
    {
      this->IdleTimer.stop();
      this->clearMotion();
      this->endInteraction();
    }
  }

  // A closed socket remains readable; without this check, the notifier loops indefinitely.
  if (this->Notifier)
  {
    char byte;
    const int result = recv(spnav_fd(), &byte, 1, MSG_PEEK | MSG_DONTWAIT);
    if (result == 0 || (result < 0 && errno != EAGAIN && errno != EWOULDBLOCK))
    {
      this->IdleTimer.stop();
      this->endInteraction();
      this->disconnectFromDaemon();
      if (this->View)
      {
        this->ReconnectTimer.start();
      }
    }
  }
}

void pqSpaceMouseImpl::clearMotion()
{
  this->FrameTimer.stop();
  this->PendingDurationMs = 0;
  this->HasPendingMotion = false;
  this->StopAfterFrame = false;
}

bool pqSpaceMouseImpl::isServerBusy(pqRenderView* view) const
{
  if (!view || !view->getServer())
  {
    return true;
  }

  auto* server = view->getServer();
  return !server->session() || server->isProgressPending() || server->isProcessingPending();
}

void pqSpaceMouseImpl::flushMotion()
{
  if (this->Interacting && (this->InteractionView != this->View || this->EndTimer.isActive()) &&
    !this->endInteraction())
  {
    this->clearMotion();
    return;
  }
  if (this->isServerBusy(this->View))
  {
    this->clearMotion();
    this->endInteraction();
    return;
  }

  if (this->HasPendingMotion && this->View)
  {
    const auto motion = this->PendingMotion;
    const double dt = this->PendingDurationMs / 1000.0;
    this->HasPendingMotion = false;
    this->PendingDurationMs = 0;
    this->moveCamera(motion, dt);
  }
  if (this->StopAfterFrame)
  {
    this->StopAfterFrame = false;
    this->endInteraction();
  }
}

void pqSpaceMouseImpl::moveCamera(const std::array<double, 6>& motion, double dt)
{
  if (!this->View || this->isServerBusy(this->View))
  {
    this->endInteraction();
    return;
  }

  vtkSMRenderViewProxy* proxy = this->View->getRenderViewProxy();
  if (!proxy)
  {
    return;
  }
  const bool is2D = vtkSMPropertyHelper(proxy, "InteractionMode").GetAsInt() ==
    vtkPVRenderView::INTERACTION_MODE_2D;
  // In 2D, keep pan, zoom and roll, but do not tilt the view out of its plane.
  if (is2D && !(motion[0] || motion[1] || motion[2] || motion[5]))
  {
    return;
  }

  vtkCamera* camera = proxy->GetActiveCamera();
  if (!camera)
  {
    return;
  }

  this->startInteraction();

  if (this->isServerBusy(this->View))
  {
    this->endInteraction();
    return;
  }

  const double distance = std::max(camera->GetDistance(), 1e-6);
  const double translation = TranslationSpeed * dt * distance / AxisRange;
  const double rotation = RotationSpeed * dt / AxisRange;

  if (is2D && motion[2] != 0.0)
  {
    // As with the 2D mouse wheel, zoom by changing parallel scale. Reciprocal
    // factors keep the scale positive in either direction.
    const double depth = motion[2] > 0.0 ? motion[2] : -motion[2];
    const double factor = 1.0 + TranslationSpeed * dt * depth / AxisRange;
    camera->SetParallelScale(
      camera->GetParallelScale() * (motion[2] > 0.0 ? factor : 1.0 / factor));
  }

  double right[3];
  double up[3];
  double forward[3];
  camera->GetDirectionOfProjection(forward);
  camera->GetViewUp(up);
  vtkMath::Cross(forward, up, right);
  vtkMath::Normalize(right);
  vtkMath::Cross(right, forward, up);
  vtkMath::Normalize(up);

  // Move the camera opposite to the puck so the model follows it. In 2D,
  // depth changes the parallel scale rather than the camera position.
  double offset[3];
  for (int i = 0; i < 3; ++i)
  {
    offset[i] = translation *
      (-motion[0] * right[i] - motion[1] * up[i] - (is2D ? 0.0 : motion[2]) * forward[i]);
  }
  double position[3];
  double focalPoint[3];
  camera->GetPosition(position);
  camera->GetFocalPoint(focalPoint);
  for (int i = 0; i < 3; ++i)
  {
    position[i] += offset[i];
    focalPoint[i] += offset[i];
  }
  camera->SetPosition(position);
  camera->SetFocalPoint(focalPoint);

  // Orbit the ParaView pivot (which need not coincide with the focal point).
  double pivot[3];
  this->View->getCenterOfRotation(pivot);
  vtkNew<vtkTransform> transform;
  transform->PostMultiply();
  transform->Translate(-pivot[0], -pivot[1], -pivot[2]);
  if (!is2D)
  {
    transform->RotateWXYZ(-motion[3] * rotation, right);
    transform->RotateWXYZ(-motion[4] * rotation, up);
  }
  transform->RotateWXYZ(-motion[5] * rotation, forward);
  transform->Translate(pivot);
  transform->TransformPoint(position, position);
  transform->TransformPoint(focalPoint, focalPoint);
  transform->TransformVector(up, up);
  camera->SetPosition(position);
  camera->SetFocalPoint(focalPoint);
  camera->SetViewUp(up);
  camera->OrthogonalizeViewUp();

  this->render();
}

void pqSpaceMouseImpl::startInteraction()
{
  if (this->Interacting)
  {
    return;
  }
  this->Interacting = true;
  this->InteractionView = this->View;
  vtkSMRenderViewProxy* proxy = this->View ? this->View->getRenderViewProxy() : nullptr;
  vtkRenderWindowInteractor* interactor = proxy ? proxy->GetInteractor() : nullptr;
  if (interactor)
  {
    interactor->InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);
  }
}

bool pqSpaceMouseImpl::endInteraction()
{
  if (!this->Interacting || !this->InteractionView)
  {
    this->EndTimer.stop();
    this->Interacting = false;
    this->InteractionView = nullptr;
    return true;
  }
  if (this->isServerBusy(this->InteractionView))
  {
    // End the interaction only when the server can perform the final still render.
    if (!this->EndTimer.isActive())
    {
      this->EndTimer.start();
    }
    return false;
  }

  pqRenderView* interactionView = this->InteractionView;
  vtkSMRenderViewProxy* proxy = interactionView->getRenderViewProxy();
  vtkRenderWindowInteractor* interactor = proxy ? proxy->GetInteractor() : nullptr;
  this->EndTimer.stop();
  this->Interacting = false;
  this->InteractionView = nullptr;
  if (interactor)
  {
    interactor->InvokeEvent(vtkCommand::EndInteractionEvent, nullptr);
  }
  else
  {
    interactionView->render();
  }
  return true;
}

void pqSpaceMouseImpl::render()
{
  if (this->isServerBusy(this->View))
  {
    this->clearMotion();
    this->endInteraction();
    return;
  }

  vtkSMRenderViewProxy* proxy = this->View ? this->View->getRenderViewProxy() : nullptr;
  vtkRenderWindowInteractor* interactor = proxy ? proxy->GetInteractor() : nullptr;
  if (interactor)
  {
    // The interactor helper selects interactive rendering while navigating.
    interactor->InvokeEvent(vtkCommand::InteractionEvent, nullptr);
    interactor->InvokeEvent(vtkCommand::RenderEvent, nullptr);
  }
  else if (this->View)
  {
    this->View->render();
  }
}
