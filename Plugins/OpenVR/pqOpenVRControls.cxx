#include "pqOpenVRControls.h"
#include "pqAnimationManager.h"
#include "pqPVApplicationCore.h"
#include "pqParaViewMenuBuilders.h"
#include "pqPipelineModel.h"
#include "pqPipelineSource.h"
#include "pqRenderView.h"
#include "pqServerManagerModel.h"
#include "pqSetName.h"
#include "pqVCRController.h"
#include "ui_pqOpenVRControls.h"
#include "vtkEventData.h"
#include "vtkOpenVRDefaultOverlay.h"
#include "vtkOpenVRInteractorStyle.h"
#include "vtkOpenVRRenderer.h"
#include "vtkPVOpenVRHelper.h"
#include "vtkRenderWindowInteractor.h"

#include <QItemSelectionModel>
#include <QMenu>
#include <QStringList>
#include <QTabWidget>

#include <functional>
#include <sstream>

class pqOpenVRControls::pqInternals : public Ui::pqOpenVRControls
{
};

void pqOpenVRControls::constructor(vtkPVOpenVRHelper* val)
{
  this->Helper = val;
  this->NoForward = false;

  this->setWindowTitle("pqOpenVRControls");
  this->setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
  // this->setTitleBarWidget(new QWidget());
  QWidget* t_widget = new QWidget(this);
  this->Internals = new pqInternals();
  this->Internals->setupUi(t_widget);
  // this->setWidget(t_widget);
  // this->hide();

  QObject::connect(this->Internals->exitButton, &QPushButton::clicked,
    std::bind(&vtkPVOpenVRHelper::Quit, this->Helper));

  QObject::connect(this->Internals->resetPositionsButton, &QPushButton::clicked, this,
    &pqOpenVRControls::resetPositions);

  QObject::connect(this->Internals->measurement, &QPushButton::clicked,
    std::bind(&vtkPVOpenVRHelper::TakeMeasurement, this->Helper));

  QObject::connect(this->Internals->comeToMeButton, &QPushButton::clicked,
    std::bind(&vtkPVOpenVRHelper::ComeToMe, this->Helper));

  QObject::connect(this->Internals->removeMeasurement, &QPushButton::clicked,
    std::bind(&vtkPVOpenVRHelper::RemoveMeasurement, this->Helper));

  QObject::connect(this->Internals->navigationPanel, &QCheckBox::stateChanged,
    [&](int state) { this->Helper->SetShowNavigationPanel(state == Qt::Checked); });

  QObject::connect(this->Internals->interactiveRay, &QCheckBox::stateChanged,
    [&](int state) { this->Helper->SetHoverPick(state == Qt::Checked); });

  QObject::connect(
    this->Internals->rightTrigger, &QComboBox::currentTextChanged, [&](QString const& text) {
      std::string mode = text.toLocal8Bit().constData();
      this->Helper->SetRightTriggerMode(mode);
    });

  QObject::connect(this->Internals->fieldValueButton, &QPushButton::clicked, this,
    &pqOpenVRControls::assignFieldValue);

  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  QList<pqRenderViewBase*> views = smmodel->findItems<pqRenderViewBase*>();
  if (!views.isEmpty())
  {
    this->Internals->pipelineBrowser->setActiveView(views[0]);
  }

  // connect crop plane buttons to the helper
  QObject::connect(this->Internals->addCropButton, &QPushButton::clicked,
    std::bind(&vtkPVOpenVRHelper::AddACropPlane, this->Helper, nullptr, nullptr));
  QObject::connect(this->Internals->addThickCropButton, &QPushButton::clicked,
    std::bind(&vtkPVOpenVRHelper::AddAThickCrop, this->Helper, nullptr));
  QObject::connect(this->Internals->removeCropsButton, &QPushButton::clicked,
    std::bind(&vtkPVOpenVRHelper::RemoveAllCropPlanesAndThickCrops, this->Helper));
  QObject::connect(this->Internals->cropSnapping, &QCheckBox::stateChanged,
    std::bind(&vtkPVOpenVRHelper::SetCropSnapping, this->Helper, std::placeholders::_1));

  QObject::connect(this->Internals->showFloorCheckbox, &QCheckBox::stateChanged, [&](int state) {
    auto ovrr = vtkOpenVRRenderer::SafeDownCast(this->Helper->GetRenderer());
    if (ovrr)
    {
      ovrr->SetShowFloor(state == Qt::Checked);
    }
  });

  QObject::connect(this->Internals->scaleFactorCombo,
    QOverload<const QString&>::of(&QComboBox::activated), [=](QString const& text) {
      if (!this->NoForward && text.length())
      {
        this->Helper->SetScaleFactor(std::stof(text.toLocal8Bit().constData()));
      }
    });

  QObject::connect(this->Internals->motionFactorCombo,
    QOverload<const QString&>::of(&QComboBox::activated), [=](QString const& text) {
      if (!this->NoForward && text.length())
      {
        this->Helper->SetMotionFactor(std::stof(text.toLocal8Bit().constData()));
      }
    });

  QObject::connect(this->Internals->loadCameraCombo,
    QOverload<const QString&>::of(&QComboBox::activated), [=](QString const& text) {
      if (!this->NoForward && text.length())
      {
        this->Helper->LoadCameraPose(std::stoi(text.toLocal8Bit().constData()));
      }
    });

  QObject::connect(this->Internals->saveCameraCombo,
    QOverload<const QString&>::of(&QComboBox::activated), [=](QString const& text) {
      if (text.length())
      {
        this->Helper->SaveCameraPose(std::stoi(text.toLocal8Bit().constData()));
      }
    });

  // Populate sources menu.
  // QMenu *menu = new QMenu();
  // pqParaViewMenuBuilders::buildSourcesMenu(*menu, nullptr);
  // this->Internals->addSource->setMenu(menu);
}

pqOpenVRControls::~pqOpenVRControls()
{
  delete this->Internals;
}

void pqOpenVRControls::resetPositions()
{
  this->Helper->ResetPositions();
}

void pqOpenVRControls::assignFieldValue()
{
  // get the current combo setting
  std::string val = this->Internals->fieldValueCombo->currentText().toLocal8Bit().constData();

  this->Helper->SetEditableFieldValue(val);
}

void pqOpenVRControls::SetRightTriggerMode(std::string const& text)
{
  this->Internals->rightTrigger->setCurrentText(text.c_str());
}

pqPipelineSource* pqOpenVRControls::GetSelectedPipelineSource()
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

void pqOpenVRControls::SetFieldValues(std::string vals)
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

void pqOpenVRControls::SetAvailablePositions(std::vector<int> const& vals)
{
  this->Internals->loadCameraCombo->clear();
  QStringList list;

  for (auto s : vals)
  {
    list << QString::number(s);
  }
  this->Internals->loadCameraCombo->addItems(list);
}

void pqOpenVRControls::SetCurrentPosition(int val)
{
  this->NoForward = true;

  auto idx = this->Internals->loadCameraCombo->findText(QString::number(val));
  this->Internals->loadCameraCombo->setCurrentIndex(idx);
  this->NoForward = false;
}

void pqOpenVRControls::SetCurrentScaleFactor(double val)
{
  this->NoForward = true;

  auto idx = this->Internals->scaleFactorCombo->findText(QString::number(val));
  this->Internals->scaleFactorCombo->setCurrentIndex(idx);
  this->NoForward = false;
}

void pqOpenVRControls::SetCurrentMotionFactor(double val)
{
  this->NoForward = true;

  auto idx = this->Internals->motionFactorCombo->findText(QString::number(val));
  this->Internals->motionFactorCombo->setCurrentIndex(idx);
  this->NoForward = false;
}
