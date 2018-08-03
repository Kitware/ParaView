#include "pvOpenVRDockPanel.h"
#include "ui_pvOpenVRDockPanel.h"

#include "pqActiveObjects.h"
#include "pqAnimationManager.h"
#include "pqAnimationScene.h"
#include "pqPVApplicationCore.h"
#include "pqRenderView.h"
#include "pqServerManagerModel.h"

#include "vtkSMRenderViewProxy.h"

#include "vtkNew.h"
#include "vtkPVOpenVRHelper.h"
#include "vtkPVRenderView.h"
#include "vtkPVXMLElement.h"
#include "vtkRenderViewBase.h"

#include <sstream>

class pvOpenVRDockPanel::pqInternals : public Ui::pvOpenVRDockPanel
{
};

void pvOpenVRDockPanel::constructor()
{
  this->Helper = vtkPVOpenVRHelper::New();
  this->setWindowTitle("OpenVR");
  QWidget* t_widget = new QWidget(this);
  this->Internals = new pqInternals();
  this->Internals->setupUi(t_widget);
  this->setWidget(t_widget);

  connect(this->Internals->sendToOpenVRButton, SIGNAL(clicked()), this, SLOT(sendToOpenVR()));
  connect(this->Internals->exportLocationsAsSkyboxesButton, SIGNAL(clicked()), this,
    SLOT(exportLocationsAsSkyboxes()));
  connect(this->Internals->exportLocationsAsViewButton, SIGNAL(clicked()), this,
    SLOT(exportLocationsAsView()));
  connect(
    this->Internals->multisamples, SIGNAL(stateChanged(int)), this, SLOT(multiSampleChanged(int)));
  connect(this->Internals->cropThickness, SIGNAL(textChanged(const QString&)), this,
    SLOT(defaultCropThicknessChanged(const QString&)));
  connect(this->Internals->editableField, SIGNAL(textChanged(const QString&)), this,
    SLOT(editableFieldChanged(const QString&)));
  connect(this->Internals->fieldValues, SIGNAL(textChanged(const QString&)), this,
    SLOT(fieldValuesChanged(const QString&)));

  // connect(
  //   &pqActiveObjects::instance(), SIGNAL(viewChanged(pqView*)), this,
  //   SLOT(setActiveView(pqView*)));

  connect(pqApplicationCore::instance(), SIGNAL(stateLoaded(vtkPVXMLElement*, vtkSMProxyLocator*)),
    this, SLOT(loadState(vtkPVXMLElement*, vtkSMProxyLocator*)));

  connect(pqApplicationCore::instance(), SIGNAL(stateSaved(vtkPVXMLElement*)), this,
    SLOT(saveState(vtkPVXMLElement*)));

  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  QObject::connect(smmodel, SIGNAL(preViewAdded(pqView*)), this, SLOT(onViewAdded(pqView*)));
  QObject::connect(smmodel, SIGNAL(preViewRemoved(pqView*)), this, SLOT(onViewRemoved(pqView*)));

  QObject::connect(
    QCoreApplication::instance(), SIGNAL(lastWindowClosed()), this, SLOT(prepareForQuit()));

  QObject::connect(pqPVApplicationCore::instance()->animationManager(), SIGNAL(beginPlay()), this,
    SLOT(beginPlay()));
  QObject::connect(
    pqPVApplicationCore::instance()->animationManager(), SIGNAL(endPlay()), this, SLOT(endPlay()));

  this->Helper->SetFieldValues(this->Internals->fieldValues->text().toLatin1().data());
}

void pvOpenVRDockPanel::sendToOpenVR()
{
  pqView* view = pqActiveObjects::instance().activeView();

  vtkSMViewProxy* smview = view->getViewProxy();
  this->Helper->SendToOpenVR(smview);
}

void pvOpenVRDockPanel::exportLocationsAsSkyboxes()
{
  pqView* view = pqActiveObjects::instance().activeView();
  vtkSMViewProxy* smview = view->getViewProxy();

  this->Helper->ExportLocationsAsSkyboxes(smview);
}

void pvOpenVRDockPanel::exportLocationsAsView()
{
  pqView* view = pqActiveObjects::instance().activeView();
  vtkSMViewProxy* smview = view->getViewProxy();

  this->Helper->ExportLocationsAsView(smview);
}

void pvOpenVRDockPanel::multiSampleChanged(int state)
{
  this->Helper->SetMultiSample(state == Qt::Checked);
}

void pvOpenVRDockPanel::defaultCropThicknessChanged(const QString& text)
{
  bool ok;
  double d = text.toDouble(&ok);
  if (ok)
  {
    this->Helper->SetDefaultCropThickness(d);
  }
}

void pvOpenVRDockPanel::editableFieldChanged(const QString& text)
{
  this->Helper->SetEditableField(text.toLatin1().data());
}

void pvOpenVRDockPanel::fieldValuesChanged(const QString& text)
{
  this->Helper->SetFieldValues(text.toLatin1().data());
}

pvOpenVRDockPanel::~pvOpenVRDockPanel()
{
  delete this->Internals;
  this->Helper->Delete();
}

//-----------------------------------------------------------------------------
void pvOpenVRDockPanel::setActiveView(pqView* /*view*/)
{
  //  pqRenderView* rview = qobject_cast<pqRenderView*>(view);

  //  vtkSMRenderViewProxy* smview = rview->getRenderViewProxy();

  //  vtkPVRenderView* cview = vtkPVRenderView::SafeDownCast(smview->GetClientSideView());

  //  if (cview)
  //  {
  // vtkNew<vtkPVOpenVRHelper> helper;
  // helper->SendToOpenVR(cview, cview->GetRenderView()->GetRenderer());
  //  }
}

void pvOpenVRDockPanel::loadState(vtkPVXMLElement* root, vtkSMProxyLocator* locator)
{
  vtkPVXMLElement* e = root->FindNestedElementByName("OpenVR");
  if (e)
  {
    int ms = 0;
    if (e->GetScalarAttribute("MultiSample", &ms))
    {
      this->Helper->SetMultiSample(ms);
      this->Internals->multisamples->setCheckState(ms ? Qt::Checked : Qt::Unchecked);
    }
    double dcropThickness = 0;
    if (e->GetScalarAttribute("DefaultCropThickness", &dcropThickness))
    {
      this->Internals->cropThickness->setText(QString::number(dcropThickness));
    }
    std::string ef = e->GetAttributeOrEmpty("EditableField");
    this->Internals->editableField->setText(QString(ef.c_str()));
    std::string fv = e->GetAttributeOrEmpty("FieldValues");
    this->Internals->fieldValues->setText(QString(fv.c_str()));

    this->Helper->LoadState(e, locator);
  }
}

void pvOpenVRDockPanel::saveState(vtkPVXMLElement* root)
{
  vtkNew<vtkPVXMLElement> e;
  e->SetName("OpenVR");

  e->AddAttribute("EditableField", this->Internals->editableField->text().toLatin1().data());

  e->AddAttribute("FieldValues", this->Internals->fieldValues->text().toLatin1().data());

  e->AddAttribute("MultiSample", this->Helper->GetMultiSample() ? 1 : 0);

  e->AddAttribute("DefaultCropThickness", this->Helper->GetDefaultCropThickness());

  this->Helper->SaveState(e);

  root->AddNestedElement(e.Get(), 1);
}

void pvOpenVRDockPanel::prepareForQuit()
{
  this->Helper->Quit();
}

void pvOpenVRDockPanel::beginPlay()
{
  pqAnimationManager* am = pqPVApplicationCore::instance()->animationManager();
  pqAnimationScene* as = am->getActiveScene();
  QObject::connect(as, SIGNAL(animationTime(double)), this, SLOT(updateSceneTime()));
}

void pvOpenVRDockPanel::endPlay()
{
  pqAnimationManager* am = pqPVApplicationCore::instance()->animationManager();
  pqAnimationScene* as = am->getActiveScene();
  QObject::disconnect(as, 0, this, 0);
}

void pvOpenVRDockPanel::updateSceneTime()
{
  this->Helper->UpdateProps();
}

//-----------------------------------------------------------------------------
void pvOpenVRDockPanel::onViewAdded(pqView*)
{
  // nothing right now
}

//-----------------------------------------------------------------------------
void pvOpenVRDockPanel::onViewRemoved(pqView* view)
{
  if (dynamic_cast<pqRenderView*>(view))
  {
    vtkSMViewProxy* smview = view->getViewProxy();

    this->Helper->ViewRemoved(smview);
  }
}
