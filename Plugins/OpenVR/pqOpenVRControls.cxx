#include "pqOpenVRControls.h"
#include "pqPVApplicationCore.h"
#include "pqParaViewMenuBuilders.h"
#include "pqPipelineModel.h"
#include "pqPipelineSource.h"
#include "pqRenderView.h"
#include "pqServerManagerModel.h"
#include "ui_pqOpenVRControls.h"
#include "vtkEventData.h"
#include "vtkOpenVRInteractorStyle.h"
#include "vtkPVOpenVRHelper.h"

#include <QItemSelectionModel>
#include <QMenu>
#include <QTabWidget>

#include <functional>

class pqOpenVRControls::pqInternals : public Ui::pqOpenVRControls
{
};

void pqOpenVRControls::constructor(vtkPVOpenVRHelper* val)
{
  this->Helper = val;

  this->setWindowTitle("pqOpenVRControls");
  this->setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
  // this->setTitleBarWidget(new QWidget());
  QWidget* t_widget = new QWidget(this);
  this->Internals = new pqInternals();
  this->Internals->setupUi(t_widget);
  // this->setWidget(t_widget);
  // this->hide();

  QObject::connect(
    this->Internals->exitButton, &QPushButton::clicked, this, &pqOpenVRControls::exit);

  QObject::connect(this->Internals->measurement, &QPushButton::clicked,
    std::bind(&vtkPVOpenVRHelper::TakeMeasurement, this->Helper));

  QObject::connect(this->Internals->removeMeasurement, &QPushButton::clicked,
    std::bind(&vtkPVOpenVRHelper::RemoveMeasurement, this->Helper));

  QObject::connect(this->Internals->controllerLabels, &QCheckBox::stateChanged, this,
    &pqOpenVRControls::controllerLabelsChanged);
  QObject::connect(this->Internals->navigationPanel, &QCheckBox::stateChanged, this,
    &pqOpenVRControls::navigationPanelChanged);

  QObject::connect(this->Internals->rightTrigger, &QComboBox::currentTextChanged, this,
    &pqOpenVRControls::rightTriggerChanged);

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
    std::bind(&vtkPVOpenVRHelper::RemoveAllCropPlanes, this->Helper));
  QObject::connect(this->Internals->cropSnapping, &QCheckBox::stateChanged,
    std::bind(&vtkPVOpenVRHelper::SetCropSnapping, this->Helper, std::placeholders::_1));

  // Populate sources menu.
  // QMenu *menu = new QMenu();
  // pqParaViewMenuBuilders::buildSourcesMenu(*menu, nullptr);
  // this->Internals->addSource->setMenu(menu);
}

pqOpenVRControls::~pqOpenVRControls()
{
  delete this->Internals;
}

void pqOpenVRControls::exit()
{
  this->Helper->Quit();
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

void pqOpenVRControls::controllerLabelsChanged(int state)
{
  this->Helper->SetDrawControls(state == Qt::Checked);
}

void pqOpenVRControls::navigationPanelChanged(int state)
{
  this->Helper->SetShowNavigationPanel(state == Qt::Checked);
}

void pqOpenVRControls::rightTriggerChanged(QString const& text)
{
  std::string mode = text.toLocal8Bit().constData();
  this->Helper->SetRightTriggerMode(mode);
}

//-----------------------------------------------------------------------------
// void pqOpenVRControls::setActiveView(pqView* view)
// {
//   this->Internals->pipelineBrowser->SetActiveView(view);
// }
