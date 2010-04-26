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
#include "vtkSmartPointer.h"
#include "vtkCamera.h"
#include "vtkProcessModule.h"
#include "vtkSMProxy.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMProperty.h"
#include "vtkSMCameraConfigurationReader.h"
#include "vtkSMCameraConfigurationWriter.h"

#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"

// Qt includes.
#include <QPointer>
#include <QString>
#include <QDebug>

// ParaView Client includes.
#include "pqApplicationCore.h"
#include "pqRenderView.h"
#include "pqPropertyLinks.h"
#include "pqSettings.h"
#include "pqFileDialog.h"
#include "pqCustomViewButtonDialog.h"

// STL
#include <vtkstd/string>
#include <vtksys/ios/sstream>

#define pqErrorMacro(estr)\
  qDebug()\
      << "Error in:" << endl\
      << __FILE__ << ", line " << __LINE__ << endl\
      << "" estr << endl;

//=============================================================================
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
    this->Internal->AutoResetCenterOfRotation,
    SIGNAL(toggled(bool)),
    this, SLOT(resetRotationCenterWithCamera()));

  QObject::connect(
    this->Internal->rollButton, SIGNAL(clicked()),
    this, SLOT(applyCameraRoll()));
  QObject::connect(
    this->Internal->elevationButton, SIGNAL(clicked()),
    this, SLOT(applyCameraElevation()));
  QObject::connect(
    this->Internal->azimuthButton, SIGNAL(clicked()),
    this, SLOT(applyCameraAzimuth()));

  QObject::connect(
    this->Internal->saveCameraConfiguration, SIGNAL(clicked()),
    this, SLOT(saveCameraConfiguration()));

  QObject::connect(
    this->Internal->loadCameraConfiguration, SIGNAL(clicked()),
    this, SLOT(loadCameraConfiguration()));

  QObject::connect(
    this->Internal->customView0, SIGNAL(clicked()),
    this, SLOT(applyCustomView0()));

  QObject::connect(
    this->Internal->customView1, SIGNAL(clicked()),
    this, SLOT(applyCustomView1()));

  QObject::connect(
    this->Internal->customView2, SIGNAL(clicked()),
    this, SLOT(applyCustomView2()));

  QObject::connect(
    this->Internal->customView3, SIGNAL(clicked()),
    this, SLOT(applyCustomView3()));

  QObject::connect(
    this->Internal->configureCustomViews, SIGNAL(clicked()),
    this, SLOT(configureCustomViews()));

  // load custom view buttons with any tool tips set by the user in a previous
  // session.
  pqCameraDialogInternal *w=this->Internal;
  pqSettings *settings=pqApplicationCore::instance()->settings();
  settings->beginGroup("CustomViewButtons");
  settings->beginGroup("ToolTips");
  w->customView0->setToolTip(settings->value("0",pqCustomViewButtonDialog::DEFAULT_TOOLTIP).toString());
  w->customView1->setToolTip(settings->value("1",pqCustomViewButtonDialog::DEFAULT_TOOLTIP).toString());
  w->customView2->setToolTip(settings->value("2",pqCustomViewButtonDialog::DEFAULT_TOOLTIP).toString());
  w->customView3->setToolTip(settings->value("3",pqCustomViewButtonDialog::DEFAULT_TOOLTIP).toString());
  settings->endGroup();
  settings->endGroup();
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

    this->Internal->position0->setValidator(
                               new QDoubleValidator(this->Internal->position0));
    this->Internal->position1->setValidator(
                               new QDoubleValidator(this->Internal->position1));
    this->Internal->position2->setValidator(
                               new QDoubleValidator(this->Internal->position2));

    this->Internal->focalPoint0->setValidator(
                             new QDoubleValidator(this->Internal->focalPoint0));
    this->Internal->focalPoint1->setValidator(
                             new QDoubleValidator(this->Internal->focalPoint1));
    this->Internal->focalPoint2->setValidator(
                             new QDoubleValidator(this->Internal->focalPoint2));

    this->Internal->viewUp0->setValidator(
                                 new QDoubleValidator(this->Internal->viewUp0));
    this->Internal->viewUp1->setValidator(
                                 new QDoubleValidator(this->Internal->viewUp1));
    this->Internal->viewUp2->setValidator(
                                 new QDoubleValidator(this->Internal->viewUp2));

    this->Internal->CenterX->setValidator(
                                 new QDoubleValidator(this->Internal->CenterX));
    this->Internal->CenterY->setValidator(
                                 new QDoubleValidator(this->Internal->CenterY));
    this->Internal->CenterZ->setValidator(
                                 new QDoubleValidator(this->Internal->CenterZ));

    this->Internal->CameraLinks.removeAllPropertyLinks();
    this->Internal->CameraLinks.addPropertyLink(
      this->Internal->position0, "text", SIGNAL(editingFinished()),
      proxy, proxy->GetProperty("CameraPosition"), 0);
    this->Internal->CameraLinks.addPropertyLink(
      this->Internal->position1, "text", SIGNAL(editingFinished()),
      proxy, proxy->GetProperty("CameraPosition"), 1);
    this->Internal->CameraLinks.addPropertyLink(
      this->Internal->position2, "text", SIGNAL(editingFinished()),
      proxy, proxy->GetProperty("CameraPosition"), 2);

    this->Internal->CameraLinks.addPropertyLink(
      this->Internal->focalPoint0, "text", SIGNAL(editingFinished()),
      proxy, proxy->GetProperty("CameraFocalPoint"), 0);
    this->Internal->CameraLinks.addPropertyLink(
      this->Internal->focalPoint1, "text", SIGNAL(editingFinished()),
      proxy, proxy->GetProperty("CameraFocalPoint"), 1);
    this->Internal->CameraLinks.addPropertyLink(
      this->Internal->focalPoint2, "text", SIGNAL(editingFinished()),
      proxy, proxy->GetProperty("CameraFocalPoint"), 2);

    this->Internal->CameraLinks.addPropertyLink(
      this->Internal->viewUp0, "text", SIGNAL(editingFinished()),
      proxy, proxy->GetProperty("CameraViewUp"), 0);
    this->Internal->CameraLinks.addPropertyLink(
      this->Internal->viewUp1, "text", SIGNAL(editingFinished()),
      proxy, proxy->GetProperty("CameraViewUp"), 1);
    this->Internal->CameraLinks.addPropertyLink(
      this->Internal->viewUp2, "text", SIGNAL(editingFinished()),
      proxy, proxy->GetProperty("CameraViewUp"), 2);

    this->Internal->CameraLinks.addPropertyLink(
      this->Internal->CenterX, "text", SIGNAL(editingFinished()),
      proxy, proxy->GetProperty("CenterOfRotation"), 0);
    this->Internal->CameraLinks.addPropertyLink(
      this->Internal->CenterY, "text", SIGNAL(editingFinished()),
      proxy, proxy->GetProperty("CenterOfRotation"), 1);
    this->Internal->CameraLinks.addPropertyLink(
      this->Internal->CenterZ, "text", SIGNAL(editingFinished()),
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
void pqCameraDialog::resetRotationCenterWithCamera()
{
  if(this->Internal->RenderModule)
    {
    this->Internal->RenderModule->setResetCenterWithCamera(
      this->Internal->AutoResetCenterOfRotation->checkState()==Qt::Checked);
    }
}

//-----------------------------------------------------------------------------
void pqCameraDialog::configureCustomViews()
{
  pqCameraDialogInternal *ui=this->Internal;

  // load the existing button configurations from the app wide settings.
  QStringList toolTips;
  QStringList configs;

  pqSettings *settings;
  settings=pqApplicationCore::instance()->settings();
  settings->beginGroup("CustomViewButtons");

  settings->beginGroup("Configurations");
  configs << settings->value("0","").toString();
  configs << settings->value("1","").toString();
  configs << settings->value("2","").toString();
  configs << settings->value("3","").toString();
  settings->endGroup();

  settings->beginGroup("ToolTips");
  toolTips << settings->value("0",ui->customView0->toolTip()).toString();
  toolTips << settings->value("1",ui->customView1->toolTip()).toString();
  toolTips << settings->value("2",ui->customView2->toolTip()).toString();
  toolTips << settings->value("3",ui->customView3->toolTip()).toString();
  settings->endGroup();
  settings->endGroup();

  // grab the current camera configuration.
  vtksys_ios::ostringstream os;

  vtkSMCameraConfigurationWriter *writer=vtkSMCameraConfigurationWriter::New();
  writer->SetRenderViewProxy(ui->RenderModule->getRenderViewProxy());
  writer->WriteConfiguration(os);

  QString currentConfig(os.str().c_str());

  // user modifies the configuration
  pqCustomViewButtonDialog dialog(this,0,toolTips,configs,currentConfig);
  if (dialog.exec()==QDialog::Accepted)
    {
    // save the new configuration into the app wide settings.
    configs=dialog.getConfigurations();
    settings->beginGroup("CustomViewButtons");
    settings->beginGroup("Configurations");
    settings->setValue("0",configs[0]);
    settings->setValue("1",configs[1]);
    settings->setValue("2",configs[2]);
    settings->setValue("3",configs[3]);
    settings->endGroup();

    toolTips=dialog.getToolTips();
    settings->beginGroup("ToolTips");
    settings->setValue("0",toolTips[0]);
    settings->setValue("1",toolTips[1]);
    settings->setValue("2",toolTips[2]);
    settings->setValue("3",toolTips[3]);
    settings->endGroup();
    settings->endGroup();

    ui->customView0->setToolTip(toolTips[0]);
    ui->customView1->setToolTip(toolTips[1]);
    ui->customView2->setToolTip(toolTips[2]);
    ui->customView3->setToolTip(toolTips[3]);
    }

  writer->Delete();
}

//-----------------------------------------------------------------------------
void pqCameraDialog::applyCustomView(int buttonId)
{
  pqCameraDialogInternal *ui=this->Internal;

  pqSettings *settings=pqApplicationCore::instance()->settings();
  settings->beginGroup("CustomViewButtons");
  settings->beginGroup("Configurations");
  QString config=settings->value(QString::number(buttonId),"").toString();
  settings->endGroup();
  settings->endGroup();

  if (!config.isEmpty())
    {
    vtkSmartPointer<vtkPVXMLParser> parser=vtkSmartPointer<vtkPVXMLParser>::New();
    parser->InitializeParser();
    parser->ParseChunk(config.toAscii().data(),static_cast<unsigned int>(config.size()));
    parser->CleanupParser();

    vtkPVXMLElement *xmlStream=parser->GetRootElement();
    if (!xmlStream)
      {
      pqErrorMacro("Invalid XML in custom view button configuration.");
      return;
      }

    vtkSmartPointer<vtkSMCameraConfigurationReader> reader
      = vtkSmartPointer<vtkSMCameraConfigurationReader>::New();

    reader->SetRenderViewProxy(ui->RenderModule->getRenderViewProxy());
    int ok=reader->ReadConfiguration(xmlStream);
    if (!ok)
      {
      pqErrorMacro(
          << "Invalid XML in custom view button "
          << buttonId << " configuration.");
      return;
      }

    // camera configuration has been modified update the scene.
    ui->RenderModule->render();
    }
}

//-----------------------------------------------------------------------------
void pqCameraDialog::saveCameraConfiguration()
{
  vtkSMCameraConfigurationWriter *writer=vtkSMCameraConfigurationWriter::New();
  writer->SetRenderViewProxy(this->Internal->RenderModule->getRenderViewProxy());

  QString filters
    = QString("%1 (*%2);;All Files (*.*)")
        .arg(writer->GetFileDescription()).arg(writer->GetFileExtension());

  pqFileDialog dialog(0,this,"Save Camera Configuration","",filters);
  dialog.setFileMode(pqFileDialog::AnyFile);

  if (dialog.exec()==QDialog::Accepted)
    {
    QString filename(dialog.getSelectedFiles()[0]);

    int ok=writer->WriteConfiguration(filename.toStdString().c_str());
    if (!ok)
      {
      pqErrorMacro("Failed to save the camera configuration.");
      }
    }

  writer->Delete();
}

//-----------------------------------------------------------------------------
void pqCameraDialog::loadCameraConfiguration()
{
  vtkSMCameraConfigurationReader *reader=vtkSMCameraConfigurationReader::New();
  reader->SetRenderViewProxy(this->Internal->RenderModule->getRenderViewProxy());

  QString filters
    = QString("%1 (*%2);;All Files (*.*)")
        .arg(reader->GetFileDescription()).arg(reader->GetFileExtension());

  pqFileDialog dialog(0,this,"Load Camera Configuration","",filters);
  dialog.setFileMode(pqFileDialog::ExistingFile);

  if (dialog.exec()==QDialog::Accepted)
    {
    QString filename;
    filename=dialog.getSelectedFiles()[0];

    int ok=reader->ReadConfiguration(filename.toStdString().c_str());
    if (!ok)
      {
      pqErrorMacro("Failed to load the camera configuration.");
      }

    // Update the scene with the new camera settings.
    this->Internal->RenderModule->render();
    }

  reader->Delete();
}
