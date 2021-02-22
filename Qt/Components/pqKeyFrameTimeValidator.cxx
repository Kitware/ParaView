/*=========================================================================

   Program: ParaView
   Module:    pqKeyFrameTimeValidator.cxx

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

=========================================================================*/
#include "pqKeyFrameTimeValidator.h"

#include "vtkEventQtSlotConnect.h"
#include "vtkSMDoubleRangeDomain.h"
#include "vtkSmartPointer.h"

#include <QPointer>

#include "pqAnimationScene.h"

//-----------------------------------------------------------------------------
class pqKeyFrameTimeValidator::pqInternals
{
public:
  vtkSmartPointer<vtkSMDoubleRangeDomain> Domain;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;
  QPointer<pqAnimationScene> AnimationScene;

  pqInternals() { this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New(); }
};

//-----------------------------------------------------------------------------
pqKeyFrameTimeValidator::pqKeyFrameTimeValidator(QObject* p)
  : QDoubleValidator(p)
{
  this->Internals = new pqInternals;
}

//-----------------------------------------------------------------------------
pqKeyFrameTimeValidator::~pqKeyFrameTimeValidator()
{
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void pqKeyFrameTimeValidator::setDomain(vtkSMDomain* domain)
{
  vtkSMDoubleRangeDomain* drd = vtkSMDoubleRangeDomain::SafeDownCast(domain);
  this->Internals->VTKConnect->Disconnect();

  this->Internals->Domain = drd;
  if (drd)
  {
    this->Internals->VTKConnect->Connect(
      drd, vtkCommand::DomainModifiedEvent, this, SLOT(onDomainModified()));
  }
  this->onDomainModified();
}

//-----------------------------------------------------------------------------
void pqKeyFrameTimeValidator::setAnimationScene(pqAnimationScene* scene)
{
  if (this->Internals->AnimationScene)
  {
    QObject::disconnect(this->Internals->AnimationScene, nullptr, this, nullptr);
  }
  this->Internals->AnimationScene = scene;
  if (scene)
  {
    QObject::connect(scene, SIGNAL(clockTimeRangesChanged()), this, SLOT(onDomainModified()),
      Qt::QueuedConnection);
  }
  this->onDomainModified();
}

//-----------------------------------------------------------------------------
void pqKeyFrameTimeValidator::onDomainModified()
{
  if (!this->Internals->Domain)
  {
    return;
  }

  double min = this->Internals->Domain->GetMinimum(0);
  double max = this->Internals->Domain->GetMaximum(0);

  if (this->Internals->AnimationScene)
  {
    QPair<double, double> range = this->Internals->AnimationScene->getClockTimeRange();
    min = range.first + (range.second - range.first) * min;
    max = range.first + (range.second - range.first) * max;
  }
  this->setRange(min, max);
}
