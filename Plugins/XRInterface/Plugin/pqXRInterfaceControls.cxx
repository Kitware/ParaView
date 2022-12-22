/*=========================================================================

  Program:   ParaView

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "pqXRInterfaceControls.h"
#include "ui_pqXRInterfaceControls.h"

#include "pqAnimationManager.h"
#include "pqPVApplicationCore.h"
#include "pqPipelineModel.h"
#include "pqPipelineSource.h"
#include "pqRenderView.h"
#include "pqServerManagerModel.h"
#include "pqSetName.h"
#include "vtkPVXRInterfaceHelper.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkVRInteractorStyle.h"
#include "vtkVRRenderer.h"

#include <QItemSelectionModel>
#include <QMenu>
#include <QStringList>

#include <functional>
#include <sstream>

#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
#define QCOMBOBOX_TEXT_ACTIVATED_SLOT QOverload<const QString&>::of(&QComboBox::activated)
#else
#define QCOMBOBOX_TEXT_ACTIVATED_SLOT &QComboBox::textActivated
#endif

//------------------------------------------------------------------------------
struct pqXRInterfaceControls::pqInternals
{
  Ui::pqXRInterfaceControls Ui;
  vtkPVXRInterfaceHelper* Helper = nullptr;
  bool NoForward = false;
};

//------------------------------------------------------------------------------
pqXRInterfaceControls::pqXRInterfaceControls(vtkPVXRInterfaceHelper* val, QWidget* parent)
  : Superclass(parent)
  , Internals(new pqXRInterfaceControls::pqInternals())
{
  this->constructor(val);
}

//------------------------------------------------------------------------------
pqXRInterfaceControls::~pqXRInterfaceControls() = default;

//------------------------------------------------------------------------------
void pqXRInterfaceControls::constructor(vtkPVXRInterfaceHelper* val)
{
  this->Internals->Helper = val;

  this->setWindowTitle("pqXRInterfaceControls");
  this->setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
  QWidget* t_widget = new QWidget(this);
  this->Internals->Ui.setupUi(t_widget);

  QObject::connect(this->Internals->Ui.exitButton, &QPushButton::clicked,
    std::bind(&vtkPVXRInterfaceHelper::Quit, this->Internals->Helper));

  QObject::connect(this->Internals->Ui.resetCameraButton, &QPushButton::clicked, this,
    &pqXRInterfaceControls::resetCamera);

  QObject::connect(this->Internals->Ui.resetPositionsButton, &QPushButton::clicked, this,
    &pqXRInterfaceControls::resetPositions);

  QObject::connect(this->Internals->Ui.measurement, &QPushButton::clicked,
    std::bind(&vtkPVXRInterfaceHelper::TakeMeasurement, this->Internals->Helper));

  QObject::connect(this->Internals->Ui.comeToMeButton, &QPushButton::clicked,
    std::bind(&vtkPVXRInterfaceHelper::ComeToMe, this->Internals->Helper));

  QObject::connect(this->Internals->Ui.removeMeasurement, &QPushButton::clicked,
    std::bind(&vtkPVXRInterfaceHelper::RemoveMeasurement, this->Internals->Helper));

  QObject::connect(this->Internals->Ui.navigationPanel, &QCheckBox::stateChanged,
    [&](int state) { this->Internals->Helper->SetShowNavigationPanel(state == Qt::Checked); });

  QObject::connect(this->Internals->Ui.interactiveRay, &QCheckBox::stateChanged,
    [&](int state) { this->Internals->Helper->SetHoverPick(state == Qt::Checked); });

  // Fill and connect Right Trigger action combo box
  this->Internals->Ui.rightTrigger->addItem(
    tr("Add Point to Source"), QVariant(vtkPVXRInterfaceHelper::ADD_POINT_TO_SOURCE));
  this->Internals->Ui.rightTrigger->addItem(tr("Grab"), QVariant(vtkPVXRInterfaceHelper::GRAB));
  this->Internals->Ui.rightTrigger->addItem(tr("Pick"), QVariant(vtkPVXRInterfaceHelper::PICK));
  this->Internals->Ui.rightTrigger->addItem(
    tr("Interactive Crop"), QVariant(vtkPVXRInterfaceHelper::INTERACTIVE_CROP));
  this->Internals->Ui.rightTrigger->addItem(tr("Probe"), QVariant(vtkPVXRInterfaceHelper::PROBE));

  QObject::connect(this->Internals->Ui.rightTrigger,
    QOverload<int>::of(&QComboBox::currentIndexChanged), [&](int index) {
      this->Internals->Helper->SetRightTriggerMode(
        this->Internals->Ui.rightTrigger->itemData(index).toInt());
    });

  // Fill and connect Movement Style combo box
  this->Internals->Ui.movementStyle->addItem(
    tr("Flying"), QVariant(vtkVRInteractorStyle::FLY_STYLE));
  this->Internals->Ui.movementStyle->addItem(
    tr("Grounded"), QVariant(vtkVRInteractorStyle::GROUNDED_STYLE));

  QObject::connect(this->Internals->Ui.movementStyle,
    QOverload<int>::of(&QComboBox::currentIndexChanged), [&](int index) {
      this->Internals->Helper->SetMovementStyle(
        this->Internals->Ui.movementStyle->itemData(index).toInt());
    });

  QObject::connect(this->Internals->Ui.fieldValueButton, &QPushButton::clicked, this,
    &pqXRInterfaceControls::assignFieldValue);

  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  QList<pqRenderViewBase*> views = smmodel->findItems<pqRenderViewBase*>();
  if (!views.isEmpty())
  {
    this->Internals->Ui.pipelineBrowser->setActiveView(views[0]);
  }

  // connect crop plane buttons to the helper
  QObject::connect(this->Internals->Ui.addCropButton, &QPushButton::clicked,
    std::bind(&vtkPVXRInterfaceHelper::AddACropPlane, this->Internals->Helper, nullptr, nullptr));
  QObject::connect(this->Internals->Ui.addThickCropButton, &QPushButton::clicked,
    std::bind(&vtkPVXRInterfaceHelper::AddAThickCrop, this->Internals->Helper, nullptr));
  QObject::connect(this->Internals->Ui.removeCropsButton, &QPushButton::clicked,
    std::bind(&vtkPVXRInterfaceHelper::RemoveAllCropPlanesAndThickCrops, this->Internals->Helper));
  QObject::connect(this->Internals->Ui.cropSnapping, &QCheckBox::stateChanged,
    std::bind(
      &vtkPVXRInterfaceHelper::SetCropSnapping, this->Internals->Helper, std::placeholders::_1));

  QObject::connect(
    this->Internals->Ui.viewUpCombo, QCOMBOBOX_TEXT_ACTIVATED_SLOT, [=](QString const& text) {
      if (!text.isEmpty())
      {
        this->Internals->Helper->SetViewUp(text.toUtf8().toStdString());
      }
    });

  QObject::connect(this->Internals->Ui.showFloorCheckbox, &QCheckBox::stateChanged, [&](int state) {
    auto ovrr = vtkVRRenderer::SafeDownCast(this->Internals->Helper->GetRenderer());
    if (ovrr)
    {
      ovrr->SetShowFloor(state == Qt::Checked);
    }
  });

  QObject::connect(
    this->Internals->Ui.scaleFactorCombo, QCOMBOBOX_TEXT_ACTIVATED_SLOT, [=](QString const& text) {
      if (!this->Internals->NoForward && !text.isEmpty())
      {
        this->Internals->Helper->SetScaleFactor(std::stof(text.toUtf8().toStdString()));
      }
    });

  QObject::connect(
    this->Internals->Ui.motionFactorCombo, QCOMBOBOX_TEXT_ACTIVATED_SLOT, [=](QString const& text) {
      if (!this->Internals->NoForward && !text.isEmpty())
      {
        this->Internals->Helper->SetMotionFactor(std::stof(text.toUtf8().toStdString()));
      }
    });

  QObject::connect(
    this->Internals->Ui.loadCameraCombo, QCOMBOBOX_TEXT_ACTIVATED_SLOT, [=](QString const& text) {
      if (!this->Internals->NoForward && !text.isEmpty())
      {
        this->Internals->Helper->LoadCameraPose(std::stoi(text.toUtf8().toStdString()));
      }
    });

  QObject::connect(
    this->Internals->Ui.saveCameraCombo, QCOMBOBOX_TEXT_ACTIVATED_SLOT, [=](QString const& text) {
      if (!text.isEmpty())
      {
        this->Internals->Helper->SaveCameraPose(std::stoi(text.toUtf8().toStdString()));
      }
    });
}

//------------------------------------------------------------------------------
void pqXRInterfaceControls::resetCamera()
{
  this->Internals->Helper->ResetCamera();
}

//------------------------------------------------------------------------------
void pqXRInterfaceControls::resetPositions()
{
  this->Internals->Helper->ResetPositions();
}

//------------------------------------------------------------------------------
void pqXRInterfaceControls::assignFieldValue()
{
  // get the current combo setting
  std::string val = this->Internals->Ui.fieldValueCombo->currentText().toUtf8().toStdString();

  this->Internals->Helper->SetEditableFieldValue(val);
}

//------------------------------------------------------------------------------
void pqXRInterfaceControls::SetRightTriggerMode(vtkPVXRInterfaceHelper::RightTriggerAction action)
{
  this->Internals->Ui.rightTrigger->setCurrentIndex(static_cast<int>(action));
}

//------------------------------------------------------------------------------
void pqXRInterfaceControls::SetMovementStyle(vtkVRInteractorStyle::MovementStyle style)
{
  this->Internals->Ui.movementStyle->setCurrentIndex(static_cast<int>(style));
}

//------------------------------------------------------------------------------
pqPipelineSource* pqXRInterfaceControls::GetSelectedPipelineSource()
{
  QItemSelectionModel* smodel = this->Internals->Ui.pipelineBrowser->getSelectionModel();
  QModelIndex selindex = smodel->currentIndex();
  if (!selindex.isValid())
  {
    return nullptr;
  }

  // Get object relative to pqPipelineModel
  const pqPipelineModel* model = this->Internals->Ui.pipelineBrowser->getPipelineModel(selindex);
  QModelIndex index = this->Internals->Ui.pipelineBrowser->pipelineModelIndex(selindex);

  // We need to obtain the source to give the undo element some sensible name.
  pqServerManagerModelItem* smModelItem = model->getItemFor(index);
  return qobject_cast<pqPipelineSource*>(smModelItem);
}

//------------------------------------------------------------------------------
void pqXRInterfaceControls::SetFieldValues(const QStringList& values)
{
  this->Internals->Ui.fieldValueCombo->clear();
  this->Internals->Ui.fieldValueCombo->addItems(values);
}

//------------------------------------------------------------------------------
void pqXRInterfaceControls::SetAvailablePositions(std::vector<int> const& vals)
{
  this->Internals->Ui.loadCameraCombo->clear();
  QStringList list;

  for (auto s : vals)
  {
    list << QString::number(s);
  }
  this->Internals->Ui.loadCameraCombo->addItems(list);
}

//------------------------------------------------------------------------------
void pqXRInterfaceControls::SetCurrentSavedPosition(int val)
{
  this->Internals->NoForward = true;

  auto idx = this->Internals->Ui.saveCameraCombo->findText(QString::number(val));
  this->Internals->Ui.saveCameraCombo->setCurrentIndex(idx);
  this->Internals->NoForward = false;
}

//------------------------------------------------------------------------------
void pqXRInterfaceControls::SetCurrentPosition(int val)
{
  this->Internals->NoForward = true;

  auto idx = this->Internals->Ui.loadCameraCombo->findText(QString::number(val));
  this->Internals->Ui.loadCameraCombo->setCurrentIndex(idx);
  this->Internals->NoForward = false;
}

//------------------------------------------------------------------------------
void pqXRInterfaceControls::SetCurrentScaleFactor(double val)
{
  this->Internals->NoForward = true;

  auto idx = this->Internals->Ui.scaleFactorCombo->findText(QString::number(val));
  this->Internals->Ui.scaleFactorCombo->setCurrentIndex(idx);
  this->Internals->NoForward = false;
}

//------------------------------------------------------------------------------
void pqXRInterfaceControls::SetCurrentMotionFactor(double val)
{
  this->Internals->NoForward = true;

  auto idx = this->Internals->Ui.motionFactorCombo->findText(QString::number(val));
  this->Internals->Ui.motionFactorCombo->setCurrentIndex(idx);
  this->Internals->NoForward = false;
}

//------------------------------------------------------------------------------
void pqXRInterfaceControls::SetCurrentViewUp(std::string dir)
{
  auto idx = this->Internals->Ui.viewUpCombo->findText(QString::fromStdString(dir));
  this->Internals->Ui.viewUpCombo->setCurrentIndex(idx);
}

//------------------------------------------------------------------------------
void pqXRInterfaceControls::SetInteractiveRay(bool val)
{
  this->Internals->Ui.interactiveRay->setCheckState(val ? Qt::Checked : Qt::Unchecked);
}

//------------------------------------------------------------------------------
void pqXRInterfaceControls::SetNavigationPanel(bool val)
{
  this->Internals->Ui.navigationPanel->setCheckState(val ? Qt::Checked : Qt::Unchecked);
}

//------------------------------------------------------------------------------
void pqXRInterfaceControls::SetSnapCropPlanes(bool checked)
{
  this->Internals->Ui.cropSnapping->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
}

//------------------------------------------------------------------------------
void pqXRInterfaceControls::SetShowFloor(bool checked)
{
  this->Internals->Ui.showFloorCheckbox->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
}
