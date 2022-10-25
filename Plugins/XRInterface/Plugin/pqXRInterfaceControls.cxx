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
#include "pqVCRController.h"
#include "vtkOpenVRInteractorStyle.h"
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
class pqXRInterfaceControls::pqInternals : public Ui::pqXRInterfaceControls
{
};

//------------------------------------------------------------------------------
void pqXRInterfaceControls::constructor(vtkPVXRInterfaceHelper* val)
{
  this->Helper = val;
  this->NoForward = false;

  this->setWindowTitle("pqXRInterfaceControls");
  this->setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
  // this->setTitleBarWidget(new QWidget());
  QWidget* t_widget = new QWidget(this);
  this->Internals = new pqInternals();
  this->Internals->setupUi(t_widget);
  // this->setWidget(t_widget);
  // this->hide();

  QObject::connect(this->Internals->exitButton, &QPushButton::clicked,
    std::bind(&vtkPVXRInterfaceHelper::Quit, this->Helper));

  QObject::connect(this->Internals->resetPositionsButton, &QPushButton::clicked, this,
    &pqXRInterfaceControls::resetPositions);

  QObject::connect(this->Internals->measurement, &QPushButton::clicked,
    std::bind(&vtkPVXRInterfaceHelper::TakeMeasurement, this->Helper));

  QObject::connect(this->Internals->comeToMeButton, &QPushButton::clicked,
    std::bind(&vtkPVXRInterfaceHelper::ComeToMe, this->Helper));

  QObject::connect(this->Internals->removeMeasurement, &QPushButton::clicked,
    std::bind(&vtkPVXRInterfaceHelper::RemoveMeasurement, this->Helper));

  QObject::connect(this->Internals->navigationPanel, &QCheckBox::stateChanged,
    [&](int state) { this->Helper->SetShowNavigationPanel(state == Qt::Checked); });

  QObject::connect(this->Internals->interactiveRay, &QCheckBox::stateChanged,
    [&](int state) { this->Helper->SetHoverPick(state == Qt::Checked); });

  QObject::connect(
    this->Internals->rightTrigger, &QComboBox::currentTextChanged, [&](QString const& text) {
      std::string mode = text.toUtf8().toStdString();
      this->Helper->SetRightTriggerMode(mode);
    });

  QObject::connect(
    this->Internals->movementStyle, &QComboBox::currentTextChanged, [&](QString const& text) {
      std::string style = text.toUtf8().toStdString();
      if (style == "Flying")
      {
        this->Helper->SetMovementStyle(vtkVRInteractorStyle::FLY_STYLE);
      }
      else if (style == "Grounded")
      {
        this->Helper->SetMovementStyle(vtkVRInteractorStyle::GROUNDED_STYLE);
      }
      else
      {
        qWarning("Unrecognised movement style.");
      }
    });

  QObject::connect(this->Internals->fieldValueButton, &QPushButton::clicked, this,
    &pqXRInterfaceControls::assignFieldValue);

  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  QList<pqRenderViewBase*> views = smmodel->findItems<pqRenderViewBase*>();
  if (!views.isEmpty())
  {
    this->Internals->pipelineBrowser->setActiveView(views[0]);
  }

  // connect crop plane buttons to the helper
  QObject::connect(this->Internals->addCropButton, &QPushButton::clicked,
    std::bind(&vtkPVXRInterfaceHelper::AddACropPlane, this->Helper, nullptr, nullptr));
  QObject::connect(this->Internals->addThickCropButton, &QPushButton::clicked,
    std::bind(&vtkPVXRInterfaceHelper::AddAThickCrop, this->Helper, nullptr));
  QObject::connect(this->Internals->removeCropsButton, &QPushButton::clicked,
    std::bind(&vtkPVXRInterfaceHelper::RemoveAllCropPlanesAndThickCrops, this->Helper));
  QObject::connect(this->Internals->cropSnapping, &QCheckBox::stateChanged,
    std::bind(&vtkPVXRInterfaceHelper::SetCropSnapping, this->Helper, std::placeholders::_1));

  QObject::connect(
    this->Internals->viewUpCombo, QCOMBOBOX_TEXT_ACTIVATED_SLOT, [=](QString const& text) {
      if (!text.isEmpty())
      {
        this->Helper->SetViewUp(text.toUtf8().toStdString());
      }
    });

  QObject::connect(this->Internals->showFloorCheckbox, &QCheckBox::stateChanged, [&](int state) {
    auto ovrr = vtkVRRenderer::SafeDownCast(this->Helper->GetRenderer());
    if (ovrr)
    {
      ovrr->SetShowFloor(state == Qt::Checked);
    }
  });

  QObject::connect(
    this->Internals->scaleFactorCombo, QCOMBOBOX_TEXT_ACTIVATED_SLOT, [=](QString const& text) {
      if (!this->NoForward && !text.isEmpty())
      {
        this->Helper->SetScaleFactor(std::stof(text.toUtf8().toStdString()));
      }
    });

  QObject::connect(
    this->Internals->motionFactorCombo, QCOMBOBOX_TEXT_ACTIVATED_SLOT, [=](QString const& text) {
      if (!this->NoForward && !text.isEmpty())
      {
        this->Helper->SetMotionFactor(std::stof(text.toUtf8().toStdString()));
      }
    });

  QObject::connect(
    this->Internals->loadCameraCombo, QCOMBOBOX_TEXT_ACTIVATED_SLOT, [=](QString const& text) {
      if (!this->NoForward && !text.isEmpty())
      {
        this->Helper->LoadCameraPose(std::stoi(text.toUtf8().toStdString()));
      }
    });

  QObject::connect(
    this->Internals->saveCameraCombo, QCOMBOBOX_TEXT_ACTIVATED_SLOT, [=](QString const& text) {
      if (!text.isEmpty())
      {
        this->Helper->SaveCameraPose(std::stoi(text.toUtf8().toStdString()));
      }
    });

  // Populate sources menu.
  // QMenu *menu = new QMenu();
  // pqParaViewMenuBuilders::buildSourcesMenu(*menu, nullptr);
  // this->Internals->addSource->setMenu(menu);
}

//------------------------------------------------------------------------------
pqXRInterfaceControls::~pqXRInterfaceControls()
{
  delete this->Internals;
}

//------------------------------------------------------------------------------
void pqXRInterfaceControls::resetPositions()
{
  this->Helper->ResetPositions();
}

//------------------------------------------------------------------------------
void pqXRInterfaceControls::assignFieldValue()
{
  // get the current combo setting
  std::string val = this->Internals->fieldValueCombo->currentText().toUtf8().toStdString();

  this->Helper->SetEditableFieldValue(val);
}

//------------------------------------------------------------------------------
void pqXRInterfaceControls::SetRightTriggerMode(std::string const& text)
{
  this->Internals->rightTrigger->setCurrentText(text.c_str());
}

//------------------------------------------------------------------------------
pqPipelineSource* pqXRInterfaceControls::GetSelectedPipelineSource()
{
  QItemSelectionModel* smodel = this->Internals->pipelineBrowser->getSelectionModel();
  QModelIndex selindex = smodel->currentIndex();
  if (!selindex.isValid())
  {
    return nullptr;
  }

  // Get object relative to pqPipelineModel
  const pqPipelineModel* model = this->Internals->pipelineBrowser->getPipelineModel(selindex);
  QModelIndex index = this->Internals->pipelineBrowser->pipelineModelIndex(selindex);

  // We need to obtain the source to give the undo element some sensible name.
  pqServerManagerModelItem* smModelItem = model->getItemFor(index);
  return qobject_cast<pqPipelineSource*>(smModelItem);
}

//------------------------------------------------------------------------------
namespace
{
std::string trim(std::string const& str)
{
  if (str.empty())
    return str;

  std::size_t firstScan = str.find_first_not_of(' ');
  std::size_t first = firstScan == std::string::npos ? str.length() : firstScan;
  std::size_t last = str.find_last_not_of(' ');
  return str.substr(first, last - first + 1);
}
}

//------------------------------------------------------------------------------
void pqXRInterfaceControls::SetFieldValues(std::string vals)
{
  this->Internals->fieldValueCombo->clear();
  QStringList list;

  std::istringstream iss(vals);

  std::string token;
  while (std::getline(iss, token, ','))
  {
    token = trim(token);
    list << QString(token.c_str());
  }

  this->Internals->fieldValueCombo->addItems(list);
}

//------------------------------------------------------------------------------
void pqXRInterfaceControls::SetAvailablePositions(std::vector<int> const& vals)
{
  this->Internals->loadCameraCombo->clear();
  QStringList list;

  for (auto s : vals)
  {
    list << QString::number(s);
  }
  this->Internals->loadCameraCombo->addItems(list);
}

//------------------------------------------------------------------------------
void pqXRInterfaceControls::SetCurrentSavedPosition(int val)
{
  this->NoForward = true;

  auto idx = this->Internals->saveCameraCombo->findText(QString::number(val));
  this->Internals->saveCameraCombo->setCurrentIndex(idx);
  this->NoForward = false;
}

//------------------------------------------------------------------------------
void pqXRInterfaceControls::SetCurrentPosition(int val)
{
  this->NoForward = true;

  auto idx = this->Internals->loadCameraCombo->findText(QString::number(val));
  this->Internals->loadCameraCombo->setCurrentIndex(idx);
  this->NoForward = false;
}

//------------------------------------------------------------------------------
void pqXRInterfaceControls::SetCurrentScaleFactor(double val)
{
  this->NoForward = true;

  auto idx = this->Internals->scaleFactorCombo->findText(QString::number(val));
  this->Internals->scaleFactorCombo->setCurrentIndex(idx);
  this->NoForward = false;
}

//------------------------------------------------------------------------------
void pqXRInterfaceControls::SetCurrentMotionFactor(double val)
{
  this->NoForward = true;

  auto idx = this->Internals->motionFactorCombo->findText(QString::number(val));
  this->Internals->motionFactorCombo->setCurrentIndex(idx);
  this->NoForward = false;
}

//------------------------------------------------------------------------------
void pqXRInterfaceControls::SetCurrentViewUp(std::string dir)
{
  auto idx = this->Internals->viewUpCombo->findText(QString::fromStdString(dir));
  this->Internals->viewUpCombo->setCurrentIndex(idx);
}
