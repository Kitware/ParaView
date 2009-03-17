/*=========================================================================

   Program: ParaView
   Module:    pqCameraPathCreator.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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
#include "pqCameraPathCreator.h"
#include "ui_pqCameraPathCreator.h"

// Server Manager Includes.
#include "vtkPoints.h"
#include "vtkSmartPointer.h"
#include "vtkSMNewWidgetRepresentationProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMUtilities.h"

// Qt Includes.
#include <QtDebug>

// ParaView Includes.
#include "pqOrbitWidget.h"
#include "pqHandleWidget.h"
#include "pqApplicationCore.h"
#include "pqServer.h"
#include "pqActiveView.h"

class pqCameraPathCreator::pqInternals : public Ui::CameraPathCreator
{
public:
  pqInternals() : SphereWidget(0), HandleWidget(0) { }
  ~pqInternals() 
    {
    delete this->SphereWidget;
    delete this->HandleWidget;
    }

  pqOrbitWidget* SphereWidget;
  pqHandleWidget* HandleWidget;
};
//-----------------------------------------------------------------------------
pqCameraPathCreator::pqCameraPathCreator(QWidget* _parent, Qt::WindowFlags _flags)
: Superclass(_parent, _flags)
{
  this->Internals = new pqInternals();
  this->Internals->setupUi(this);

  QObject::connect(this->Internals->FixedLocationGroup, SIGNAL(toggled(bool)),
    this, SLOT(fixedLocation(bool)));
  QObject::connect(this->Internals->OrbitGroup, SIGNAL(toggled(bool)),
    this, SLOT(orbit(bool)));

  pqServer* server = pqApplicationCore::instance()->getActiveServer();
  if (!server)
    {
    qCritical() << "pqCameraPathCreator needs an active server to create the 3D"
      " widgets used to sets the values";
    return;
    }

  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  vtkSMProxy* dummy = pxm->NewProxy("implicit_functions", "None");
  dummy->SetConnectionID(server->GetConnectionID());

  this->Internals->HandleWidget = new pqHandleWidget(dummy, dummy,
    this->Internals->FixedLocationGroup);
  (new QVBoxLayout(this->Internals->FixedLocationGroup))->addWidget(
    this->Internals->HandleWidget);
  this->Internals->FixedLocationGroup->layout()->setMargin(0);
  QObject::connect(&pqActiveView::instance(), SIGNAL(changed(pqView*)),
    this->Internals->HandleWidget, SLOT(setView(pqView*)));
  this->Internals->HandleWidget->setUseSelectionDataBounds(true);
  this->Internals->HandleWidget->setView(pqActiveView::instance().current());

  this->Internals->SphereWidget = new pqOrbitWidget(dummy, dummy,
    this->Internals->OrbitGroup);
  (new QVBoxLayout(this->Internals->OrbitGroup))->addWidget(
    this->Internals->SphereWidget);
  this->Internals->OrbitGroup->layout()->setMargin(0);
  QObject::connect(&pqActiveView::instance(), SIGNAL(changed(pqView*)),
    this->Internals->SphereWidget, SLOT(setView(pqView*)));
  this->Internals->SphereWidget->setView(pqActiveView::instance().current());
  this->Internals->SphereWidget->setUseSelectionDataBounds(true);
  this->Internals->SphereWidget->resetBounds();
  dummy->Delete();

  this->fixedLocation(false);
}

//-----------------------------------------------------------------------------
pqCameraPathCreator::~pqCameraPathCreator()
{
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void pqCameraPathCreator::fixedLocation(bool val)
{
  this->setMode(val? FIXED_LOCATION : ORBIT);
}

//-----------------------------------------------------------------------------
void pqCameraPathCreator::orbit(bool val)
{
  this->setMode(val? ORBIT : FIXED_LOCATION);
}

//-----------------------------------------------------------------------------
void pqCameraPathCreator::setMode(int mode)
{
  switch (mode)
    {
  case ORBIT:
    this->Internals->SphereWidget->select();
    this->Internals->HandleWidget->deselect();
    this->Internals->OrbitGroup->setChecked(true);
    this->Internals->FixedLocationGroup->setChecked(false);
    break;

  default:
    this->Internals->SphereWidget->deselect();
    this->Internals->HandleWidget->select();
    this->Internals->OrbitGroup->setChecked(false);
    this->Internals->FixedLocationGroup->setChecked(true);
    break;
    }
  this->Mode = mode;
}

//-----------------------------------------------------------------------------
QList<QVariant> pqCameraPathCreator::createOrbit(const double center[3],
  const double n[3], double radius, int resolution)
{
  QList<QVariant> points;
  vtkPoints* pts = vtkSMUtilities::CreateOrbit(
    center, n, radius, resolution);
  for (vtkIdType cc=0; cc < pts->GetNumberOfPoints(); cc++)
    {
    double coords[3];
    pts->GetPoint(cc, coords);
    points << coords[0] << coords[1] << coords[2];
    }
  pts->Delete();
  return points;
}

//-----------------------------------------------------------------------------
void pqCameraPathCreator::done(int r)
{
  if (r == QDialog::Accepted)
    {
    if (this->Mode == ORBIT)
      {
      vtkSMProxy* widget = this->Internals->SphereWidget->getWidgetProxy();
      const double *ptr_normal = vtkSMPropertyHelper(widget,
        "HandleDirection").GetAsDoublePtr();
      const QList<QVariant> points = this->createOrbit(
        vtkSMPropertyHelper(widget, "Center").GetAsDoublePtr(),
        ptr_normal,
        vtkSMPropertyHelper(widget, "Radius").GetAsDouble(),
        20);
      QList<QVariant> normal;
      normal << ptr_normal[0] << ptr_normal[1] << ptr_normal[2];
      emit this->pathSelected(points);
      emit this->pathNormal(normal);
      emit this->closedPath(true);
      }
    }
  this->Superclass::done(r);
}

