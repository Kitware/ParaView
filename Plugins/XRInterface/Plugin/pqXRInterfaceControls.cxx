// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pqXRInterfaceControls.h"
#include "ui_pqXRInterfaceControls.h"

#include "pqAnimationManager.h"
#include "pqCustomViewpointsToolbar.h"
#include "pqPVApplicationCore.h"
#include "pqPipelineModel.h"
#include "pqPipelineSource.h"
#include "pqRenderView.h"
#include "pqServerManagerModel.h"
#include "pqSetName.h"
#include "pqXRCustomViewpointsController.h"
#include "vtkPVXRInterfaceHelper.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkVRInteractorStyle.h"
#include "vtkVRRenderer.h"

#include <QButtonGroup>
#include <QItemSelectionModel>
#include <QMenu>
#include <QStringList>

#include <functional>
#include <sstream>

// Name of view up used for view up button group
namespace
{
constexpr std::array<const char*, 6> viewUpNames = { "-X", "+X", "-Y", "+Y", "-Z", "+Z" };
constexpr std::array<double, 7> sceneScaleButtonValues = { 0.001, 0.01, 0.1, 1.0, 10.0, 100.0,
  1000.0 };
}

//------------------------------------------------------------------------------
struct pqXRInterfaceControls::pqInternals
{
  Ui::pqXRInterfaceControls Ui;
  QButtonGroup* ViewUpGroup = nullptr;
  vtkPVXRInterfaceHelper* Helper = nullptr;
  pqCustomViewpointsToolbar* CustomViewpointsToolbar = nullptr;
  bool NoForward = false;
};

//------------------------------------------------------------------------------
pqXRInterfaceControls::pqXRInterfaceControls(vtkPVXRInterfaceHelper* val, QWidget* parent)
  : Superclass(parent)
  , Internals(new pqXRInterfaceControls::pqInternals())
{
  this->constructor(val);

  auto* controller = new pqXRCustomViewpointsController(this->Internals->Helper, this);
  auto* toolbar = new pqCustomViewpointsToolbar(controller, this);
  toolbar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

  const auto getAction = [toolbar](const QString& name) -> QAction*
  {
    for (auto action : toolbar->actions())
    {
      if (action->objectName() == name)
      {
        return action;
      }
    }

    return toolbar->actions()[0];
  };

  getAction("ConfigAction")->setText("Clear");
  getAction("PlusAction")->setText("Save pose");

  this->Internals->Ui.movementPageVLayout->insertWidget(4, toolbar);
  this->Internals->Ui.pqXRInterfaceTabs->tabBar()->setExpanding(true);

  this->Internals->CustomViewpointsToolbar = toolbar;
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
    [this]() { this->Internals->Helper->Quit(); });

  QObject::connect(this->Internals->Ui.resetCameraButton, &QPushButton::clicked, this,
    &pqXRInterfaceControls::resetCamera);

  QObject::connect(this->Internals->Ui.resetPositionsButton, &QPushButton::clicked, this,
    &pqXRInterfaceControls::resetPositions);

  QObject::connect(this->Internals->Ui.rulerButton, &QPushButton::toggled,
    [&](bool checked)
    {
      if (checked)
      {
        this->Internals->Helper->TakeMeasurement();
      }
      else
      {
        this->Internals->Helper->RemoveMeasurement();
      }
    });

  QObject::connect(this->Internals->Ui.comeToMeButton, &QPushButton::clicked,
    [this]() { this->Internals->Helper->ComeToMe(); });

  QObject::connect(this->Internals->Ui.navigationPanel, &QPushButton::toggled,
    [&](bool checked) { this->Internals->Helper->SetShowNavigationPanel(checked); });

  QObject::connect(this->Internals->Ui.interactiveRay, &QPushButton::toggled,
    [&](bool checked) { this->Internals->Helper->SetHoverPick(checked); });

  // Fill and connect Right Trigger action combo box
  this->Internals->Ui.rightTrigger->addItem(
    tr("Add Point to Source"), QVariant(vtkPVXRInterfaceHelper::ADD_POINT_TO_SOURCE));
  this->Internals->Ui.rightTrigger->addItem(tr("Grab"), QVariant(vtkPVXRInterfaceHelper::GRAB));
  this->Internals->Ui.rightTrigger->addItem(tr("Pick"), QVariant(vtkPVXRInterfaceHelper::PICK));
  this->Internals->Ui.rightTrigger->addItem(
    tr("Interactive Crop"), QVariant(vtkPVXRInterfaceHelper::INTERACTIVE_CROP));
  this->Internals->Ui.rightTrigger->addItem(tr("Probe"), QVariant(vtkPVXRInterfaceHelper::PROBE));
  this->Internals->Ui.rightTrigger->addItem(
    tr("Teleportation"), QVariant(vtkPVXRInterfaceHelper::TELEPORTATION));

  QObject::connect(this->Internals->Ui.rightTrigger,
    QOverload<int>::of(&QComboBox::currentIndexChanged),
    [&](int index)
    {
      this->Internals->Helper->SetRightTriggerMode(
        this->Internals->Ui.rightTrigger->itemData(index).toInt());
    });

  // Fill and connect Movement Style combo box
  this->Internals->Ui.movementStyle->addItem(
    tr("Flying"), QVariant(vtkVRInteractorStyle::FLY_STYLE));
  this->Internals->Ui.movementStyle->addItem(
    tr("Grounded"), QVariant(vtkVRInteractorStyle::GROUNDED_STYLE));

  QObject::connect(this->Internals->Ui.movementStyle,
    QOverload<int>::of(&QComboBox::currentIndexChanged),
    [&](int index)
    {
      this->Internals->Helper->SetMovementStyle(
        this->Internals->Ui.movementStyle->itemData(index).toInt());
    });

  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  QList<pqRenderViewBase*> views = smmodel->findItems<pqRenderViewBase*>();
  if (!views.isEmpty())
  {
    this->Internals->Ui.pipelineBrowser->setActiveView(views[0]);
  }

  // connect crop plane buttons to the helper
  QObject::connect(this->Internals->Ui.addCropButton, &QPushButton::clicked,
    [this]() { this->Internals->Helper->AddACropPlane(nullptr, nullptr); });
  QObject::connect(this->Internals->Ui.addThickCropButton, &QPushButton::clicked,
    [this]() { this->Internals->Helper->AddAThickCrop(nullptr); });

  QObject::connect(this->Internals->Ui.hideCropPlanesButton, &QPushButton::toggled,
    [&](bool checked) { this->Internals->Helper->ShowCropPlanes(!checked); });

  QObject::connect(this->Internals->Ui.removeCropsButton, &QPushButton::clicked,
    [this]() { this->Internals->Helper->RemoveAllCropPlanesAndThickCrops(); });
  QObject::connect(this->Internals->Ui.cropSnapping, &QPushButton::toggled,
    [&](bool checked) { this->Internals->Helper->SetCropSnapping(checked); });

  this->Internals->ViewUpGroup = new QButtonGroup{ this };
  this->Internals->ViewUpGroup->addButton(this->Internals->Ui.viewMinusX, 0);
  this->Internals->ViewUpGroup->addButton(this->Internals->Ui.viewPlusX, 1);
  this->Internals->ViewUpGroup->addButton(this->Internals->Ui.viewMinusY, 2);
  this->Internals->ViewUpGroup->addButton(this->Internals->Ui.viewPlusY, 3);
  this->Internals->ViewUpGroup->addButton(this->Internals->Ui.viewMinusZ, 4);
  this->Internals->ViewUpGroup->addButton(this->Internals->Ui.viewPlusZ, 5);

  QObject::connect(this->Internals->ViewUpGroup,
    QOverload<QAbstractButton*>::of(&QButtonGroup::buttonClicked),
    [this](QAbstractButton* button)
    { this->Internals->Helper->SetViewUp(viewUpNames[this->Internals->ViewUpGroup->id(button)]); });

  QObject::connect(this->Internals->Ui.showFloorButton, &QPushButton::toggled,
    [&](bool checked)
    {
      auto ovrr = vtkVRRenderer::SafeDownCast(this->Internals->Helper->GetRenderer());
      if (ovrr)
      {
        ovrr->SetShowFloor(checked);
      }
    });

  auto* scaleGroup = new QButtonGroup{ this };
  // use ID to store buttons' scale value
  scaleGroup->addButton(this->Internals->Ui.scaleButton_0001, 0);
  scaleGroup->addButton(this->Internals->Ui.scaleButton_001, 1);
  scaleGroup->addButton(this->Internals->Ui.scaleButton_01, 2);
  scaleGroup->addButton(this->Internals->Ui.scaleButton_1, 3);
  scaleGroup->addButton(this->Internals->Ui.scaleButton_10, 4);
  scaleGroup->addButton(this->Internals->Ui.scaleButton_100, 5);
  scaleGroup->addButton(this->Internals->Ui.scaleButton_1000, 6);

  QObject::connect(scaleGroup, QOverload<QAbstractButton*>::of(&QButtonGroup::buttonClicked),
    [this, scaleGroup](QAbstractButton* button)
    {
      if (!this->Internals->NoForward)
      {
        this->Internals->Helper->SetScaleFactor(sceneScaleButtonValues[scaleGroup->id(button)]);
      }
    });

  QObject::connect(this->Internals->Ui.movementSpeedSlider, &QSlider::valueChanged,
    [=](int value)
    {
      if (!this->Internals->NoForward)
      {
        const auto scale = std::pow(10.0, (value - 50.0) / 25.0); // [0; 100] -> [0.01; 100]
        const auto precision = 2 - static_cast<int>(std::log(scale) / std::log(10.0));
        this->Internals->Ui.movementSpeedLabel->setText(
          tr("Movement speed: x%1").arg(scale, 0, 'f', precision));
        this->Internals->Helper->SetMotionFactor(scale);
      }
    });

  QObject::connect(this->Internals->Ui.cropThicknessSlider, &QSlider::valueChanged,
    [=](int value)
    {
      if (!this->Internals->NoForward)
      {
        this->Internals->Helper->SetDefaultCropThickness(value);

        if (value == 0)
        {
          this->Internals->Ui.cropThicknessLabel->setText(tr("Crop Thickness: Auto"));
        }
        else
        {
          this->Internals->Ui.cropThicknessLabel->setText(tr("Crop Thickness: x%1").arg(value));
        }
      }
    });

  // This centers the toolbar actions
  auto leftSpacer = new QWidget(this);
  leftSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  leftSpacer->setVisible(true);
  this->Internals->Ui.VCR->insertWidget(this->Internals->Ui.VCR->actions()[0], leftSpacer);
  auto rightSpacer = new QWidget(this);
  rightSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  rightSpacer->setVisible(true);
  this->Internals->Ui.VCR->addWidget(rightSpacer);

#ifndef XRINTERFACE_HAS_COLLABORATION
  this->Internals->Ui.comeToMeButton->setVisible(false);
#endif
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
void pqXRInterfaceControls::SetCurrentMotionFactor(double val)
{
  this->Internals->NoForward = true;

  const auto scale = std::min(std::max(val, 0.01), 100.0); // clamp [0.01; 100]
  const auto logScale = std::log(scale) / std::log(10.0);
  const auto sliderValue = static_cast<int>(25.0 * logScale + 50.0); // [0.01; 100] -> [0; 100]
  const auto precision = 2 - static_cast<int>(logScale);

  this->Internals->Ui.movementSpeedLabel->setText(
    tr("Movement speed: x%1").arg(scale, 0, 'f', precision));
  this->Internals->Ui.movementSpeedSlider->setValue(sliderValue);

  this->Internals->NoForward = false;
}

//------------------------------------------------------------------------------
void pqXRInterfaceControls::SetCurrentViewUp(std::string dir)
{
  const auto it = std::find_if(
    viewUpNames.begin(), viewUpNames.end(), [&dir](const char* str) { return dir == str; });

  if (it == viewUpNames.end())
  {
    return;
  }

  this->Internals->NoForward = true;
  auto* button = this->Internals->ViewUpGroup->button(std::distance(viewUpNames.begin(), it));
  button->setChecked(true);
  this->Internals->NoForward = false;
}

//------------------------------------------------------------------------------
void pqXRInterfaceControls::SetInteractiveRay(bool val)
{
  this->Internals->Ui.interactiveRay->setChecked(val);
}

//------------------------------------------------------------------------------
void pqXRInterfaceControls::SetNavigationPanel(bool val)
{
  this->Internals->Ui.navigationPanel->setChecked(val);
}

//------------------------------------------------------------------------------
void pqXRInterfaceControls::SetSnapCropPlanes(bool checked)
{
  this->Internals->Ui.cropSnapping->setChecked(checked);
}

//------------------------------------------------------------------------------
void pqXRInterfaceControls::SetShowFloor(bool checked)
{
  this->Internals->Ui.showFloorButton->setChecked(checked);
}

//------------------------------------------------------------------------------
void pqXRInterfaceControls::UpdateCustomViewpointsToolbar()
{
  this->Internals->CustomViewpointsToolbar->updateCustomViewpointActions();
}
