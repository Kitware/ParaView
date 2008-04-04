/*=========================================================================

   Program: ParaView
   Module:    pqCameraDialog.cxx

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
#include "pqCameraDialog.h"
#include "ui_pqCameraDialog.h"

// ParaView Server Manager includes.
#include "vtkCamera.h"
#include "vtkProcessModule.h"
#include "vtkSMProxy.h"
#include "vtkSMRenderViewProxy.h"

// Qt includes.
#include <QPointer>

// ParaView Client includes.
#include "pqApplicationCore.h"
#include "pqRenderView.h"
#include "pqPropertyLinks.h"

//-----------------------------------------------------------------------------
class pqCameraDialogInternal : public Ui::pqCameraDialog
{
public:
  QPointer<pqRenderView> RenderModule;
  pqPropertyLinks CameraLinks;

  pqCameraDialogInternal()
    {
    }
  ~pqCameraDialogInternal()
    {
    }
};

//-----------------------------------------------------------------------------
pqCameraDialog::pqCameraDialog(QWidget* _p/*=null*/, 
  Qt::WFlags f/*=0*/): pqDialog(_p, f)
{
  this->Internal = new pqCameraDialogInternal;
  this->Internal->setupUi(this);

  this->setUndoLabel("Camera");

  QObject::connect(
    this->Internal->viewXPlus, SIGNAL(clicked()),
    this, SLOT(resetViewDirectionPosX()));
  QObject::connect(
    this->Internal->viewXMinus, SIGNAL(clicked()),
    this, SLOT(resetViewDirectionNegX()));
  QObject::connect(
    this->Internal->viewYPlus, SIGNAL(clicked()),
    this, SLOT(resetViewDirectionPosY()));
  QObject::connect(
    this->Internal->viewYMinus, SIGNAL(clicked()),
    this, SLOT(resetViewDirectionNegY()));  
  QObject::connect(
    this->Internal->viewZPlus, SIGNAL(clicked()),
    this, SLOT(resetViewDirectionPosZ()));
  QObject::connect(
    this->Internal->viewZMinus, SIGNAL(clicked()),
    this, SLOT(resetViewDirectionNegZ()));

  QObject::connect(
    this->Internal->CustomCenter,
    SIGNAL(toggled(bool)),
    this, SLOT(useCustomRotationCenter()));
  QObject::connect(
    this->Internal->AutoResetCenterOfRotation,
    SIGNAL(toggled(bool)),
    this, SLOT(resetRotationCenterWithCamera()));
  QObject::connect(
    this->Internal->CenterX,
    SIGNAL(valueChanged(double)),
    this, SLOT(applyRotationCenter()));
  QObject::connect(
    this->Internal->CenterY,
    SIGNAL(valueChanged(double)),
    this, SLOT(applyRotationCenter()));
  QObject::connect(
    this->Internal->CenterZ,
    SIGNAL(valueChanged(double)),
    this, SLOT(applyRotationCenter()));

  QObject::connect(
    this->Internal->rollButton, SIGNAL(clicked()),
    this, SLOT(applyCameraRoll()));
  QObject::connect(
    this->Internal->elevationButton, SIGNAL(clicked()),
    this, SLOT(applyCameraElevation()));
  QObject::connect(
    this->Internal->azimuthButton, SIGNAL(clicked()),
    this, SLOT(applyCameraAzimuth()));
}

//-----------------------------------------------------------------------------
pqCameraDialog::~pqCameraDialog()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqCameraDialog::setRenderModule(pqRenderView* ren)
{
  this->Internal->RenderModule = ren;
  this->setupGUI();
}

//-----------------------------------------------------------------------------
void pqCameraDialog::setupGUI()
{
  if (this->Internal->RenderModule)
    {
    vtkSMRenderViewProxy* proxy = 
      this->Internal->RenderModule->getRenderViewProxy();
    proxy->SynchronizeCameraProperties();

    this->Internal->CameraLinks.removeAllPropertyLinks();
    this->Internal->CameraLinks.addPropertyLink(
      this->Internal->position0, "value", SIGNAL(valueChanged(double)),
      proxy, proxy->GetProperty("CameraPosition"), 0);
    this->Internal->CameraLinks.addPropertyLink(
      this->Internal->position1, "value", SIGNAL(valueChanged(double)),
      proxy, proxy->GetProperty("CameraPosition"), 1);
    this->Internal->CameraLinks.addPropertyLink(
      this->Internal->position2, "value", SIGNAL(valueChanged(double)),
      proxy, proxy->GetProperty("CameraPosition"), 2);

    this->Internal->CameraLinks.addPropertyLink(
      this->Internal->focalPoint0, "value", SIGNAL(valueChanged(double)),
      proxy, proxy->GetProperty("CameraFocalPoint"), 0);
    this->Internal->CameraLinks.addPropertyLink(
      this->Internal->focalPoint1, "value", SIGNAL(valueChanged(double)),
      proxy, proxy->GetProperty("CameraFocalPoint"), 1);
    this->Internal->CameraLinks.addPropertyLink(
      this->Internal->focalPoint2, "value", SIGNAL(valueChanged(double)),
      proxy, proxy->GetProperty("CameraFocalPoint"), 2);

    this->Internal->CameraLinks.addPropertyLink(
      this->Internal->viewUp0, "value", SIGNAL(valueChanged(double)),
      proxy, proxy->GetProperty("CameraViewUp"), 0);
    this->Internal->CameraLinks.addPropertyLink(
      this->Internal->viewUp1, "value", SIGNAL(valueChanged(double)),
      proxy, proxy->GetProperty("CameraViewUp"), 1);  
    this->Internal->CameraLinks.addPropertyLink(
      this->Internal->viewUp2, "value", SIGNAL(valueChanged(double)),
      proxy, proxy->GetProperty("CameraViewUp"), 2);

    this->Internal->CameraLinks.addPropertyLink(
      this->Internal->CenterX, "value", SIGNAL(valueChanged(double)),
      proxy, proxy->GetProperty("CenterOfRotation"), 0);
    this->Internal->CameraLinks.addPropertyLink(
      this->Internal->CenterY, "value", SIGNAL(valueChanged(double)),
      proxy, proxy->GetProperty("CenterOfRotation"), 1);  
    this->Internal->CameraLinks.addPropertyLink(
      this->Internal->CenterZ, "value", SIGNAL(valueChanged(double)),
      proxy, proxy->GetProperty("CenterOfRotation"), 2);

    this->Internal->CameraLinks.addPropertyLink(
      this->Internal->viewAngle, "value", SIGNAL(valueChanged(double)),
      proxy, proxy->GetProperty("CameraViewAngle"), 0);

    QObject::connect(&this->Internal->CameraLinks, SIGNAL(qtWidgetChanged()),
      this->Internal->RenderModule, SLOT(render()));

    this->Internal->AutoResetCenterOfRotation->setCheckState(
      this->Internal->RenderModule->getResetCenterWithCamera()? Qt::Checked : Qt::Unchecked);
    }
}

//-----------------------------------------------------------------------------
void pqCameraDialog::SetCameraGroupsEnabled(bool enabled)
{
  this->Internal->viewsGroup->setEnabled(enabled);
  this->Internal->positionsGroup->setEnabled(enabled);
  this->Internal->orientationsGroup->setEnabled(enabled);
}

//-----------------------------------------------------------------------------
void pqCameraDialog::resetViewDirection(
    double look_x, double look_y, double look_z,
    double up_x, double up_y, double up_z)
{
  if (this->Internal->RenderModule)
    {
    this->Internal->RenderModule->resetViewDirection(
      look_x, look_y, look_z, up_x, up_y, up_z);
    }
}

//-----------------------------------------------------------------------------
void pqCameraDialog::resetViewDirectionPosX()
{
  this->resetViewDirection(1, 0, 0, 0, 0, 1);
}
//-----------------------------------------------------------------------------
void pqCameraDialog::resetViewDirectionNegX()
{
  this->resetViewDirection(-1, 0, 0, 0, 0, 1);

}

//-----------------------------------------------------------------------------
void pqCameraDialog::resetViewDirectionPosY()
{
  this->resetViewDirection(0, 1, 0, 0, 0, 1);
}

//-----------------------------------------------------------------------------
void pqCameraDialog::resetViewDirectionNegY()
{
  this->resetViewDirection(0, -1, 0, 0, 0, 1);
}

//-----------------------------------------------------------------------------
void pqCameraDialog::resetViewDirectionPosZ()
{
  this->resetViewDirection(0, 0, 1, 0, 1, 0);
}

//-----------------------------------------------------------------------------
void pqCameraDialog::resetViewDirectionNegZ()
{
  this->resetViewDirection(0, 0, -1, 0, 1, 0);
}

//-----------------------------------------------------------------------------
void pqCameraDialog::adjustCamera(
  CameraAdjustmentType enType, double angle)
{
  if (this->Internal->RenderModule)
    {
    vtkSMRenderViewProxy* proxy = 
      this->Internal->RenderModule->getRenderViewProxy();
    proxy->SynchronizeCameraProperties();
    vtkCamera* camera = proxy->GetActiveCamera();
    if (!camera)
      {
      return;
      }
    if(enType == pqCameraDialog::Roll)
      {
      camera->Roll(angle);
      }
    else if(enType == pqCameraDialog::Elevation)
      {
      camera->Elevation(angle);
      }
    else if(enType == pqCameraDialog::Azimuth)
      {
      camera->Azimuth(angle);
      }
    proxy->SynchronizeCameraProperties();
    this->Internal->RenderModule->render();
    }
}

//-----------------------------------------------------------------------------
void pqCameraDialog::applyCameraRoll()
{
  this->adjustCamera(pqCameraDialog::Roll, 
    this->Internal->rollAngle->value());
}

//-----------------------------------------------------------------------------
void pqCameraDialog::applyCameraElevation()
{
  this->adjustCamera(pqCameraDialog::Elevation, 
    this->Internal->elevationAngle->value());
}

//-----------------------------------------------------------------------------
void pqCameraDialog::applyCameraAzimuth()
{
  this->adjustCamera(pqCameraDialog::Azimuth, 
    this->Internal->azimuthAngle->value());
}

//-----------------------------------------------------------------------------
void pqCameraDialog::useCustomRotationCenter()
{
  if(this->Internal->CustomCenter->checkState() == Qt::Checked)
    {
    this->resetRotationCenter();
    }
}

//-----------------------------------------------------------------------------
void pqCameraDialog::applyRotationCenter()
{
  if(!this->Internal->RenderModule)
    {
    return;
    }

  if (this->Internal->CustomCenter->checkState() == Qt::Checked)
    {
    double center[3];
    center[0] = this->Internal->CenterX->text().toDouble();
    center[1] = this->Internal->CenterY->text().toDouble();
    center[2] = this->Internal->CenterZ->text().toDouble();
    this->Internal->RenderModule->setCenterOfRotation(center);
    }

  // update the view after changes
  this->Internal->RenderModule->render();
}

//-----------------------------------------------------------------------------
void pqCameraDialog::resetRotationCenterWithCamera()
{
  if(this->Internal->RenderModule)
    {
    this->Internal->RenderModule->setResetCenterWithCamera(
      this->Internal->AutoResetCenterOfRotation->checkState()==Qt::Checked);
    }
}

//-----------------------------------------------------------------------------
void pqCameraDialog::resetRotationCenter()
{
  if(this->Internal->RenderModule)
    {
    double center[3];
    this->Internal->RenderModule->getCenterOfRotation(center);
    this->Internal->CenterX->setValue(center[0]);
    this->Internal->CenterY->setValue(center[1]);
    this->Internal->CenterZ->setValue(center[2]);
    }
}
