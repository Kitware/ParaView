// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pqVRCollaborationWidget.h"
#include "ui_pqVRCollaborationWidget.h"

#include "pqApplicationCore.h"
#include "pqVRQueueHandler.h"
#include "pqView.h"
#include "vtkCommand.h"
#include "vtkOpenGLRenderer.h"
#include "vtkPVRenderView.h"
#include "vtkPVXMLElement.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkStringList.h"

#include <QComboBox>
#include <QDebug>
#include <QStringList>

#include <algorithm>
#include <map>
#include <string>

#if CAVEINTERACTION_HAS_COLLABORATION

#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
#define pqCheckBoxSignal checkStateChanged
using pqCheckState = Qt::CheckState;
#else
#define pqCheckBoxSignal stateChanged
using pqCheckState = int;
#endif

#include "pqVRAvatarEvents.h"
#include "vtkSMVRCollaborationStyleProxy.h"
#include "vtkVRCollaborationClient.h"
class pqVRCollaborationWidget::pqInternals : public Ui::VRCollaborationWidget
{
public:
  /*
   * Event handler allowing the collaboration client an opportunity to perform
   * communication and update avatar actors.
   */
  class vtkStartRenderObserver : public vtkCommand
  {
  public:
    static vtkStartRenderObserver* New() { return new vtkStartRenderObserver; }

    void Execute(vtkObject* vtkNotUsed(caller), unsigned long vtkNotUsed(event),
      void* vtkNotUsed(calldata)) override
    {
      this->CollaborationClient->Render();
    }

    vtkVRCollaborationClient* CollaborationClient;

    void SetCollaborationClient(vtkVRCollaborationClient* client)
    {
      this->CollaborationClient = client;
    }

  protected:
    vtkStartRenderObserver() { this->CollaborationClient = nullptr; }
    ~vtkStartRenderObserver() override {}
  };

  /*
   * Allows redirecting output messages from vtkVRCollaborationClient into a
   * component in the UI.
   */
  void collaborationLoggingCallback(std::string const& msg, vtkLogger::Verbosity /*verbosity*/)
  {
    // send message if any to text window
    if (!msg.length())
    {
      return;
    }

    this->outputWindow->appendPlainText(msg.c_str());
  }

  /*
   * Convenience method to try and retrieve the vtkOpenGLRenderer from the
   * given pqView.
   */
  vtkOpenGLRenderer* GetOpenGLRenderer(pqView* view)
  {
    vtkOpenGLRenderer* oglRen = nullptr;
    if (view)
    {
      vtkSMRenderViewProxy* proxy = vtkSMRenderViewProxy::SafeDownCast(view->getViewProxy());
      if (proxy)
      {
        // We need the local renderer being used on the paraview client.
        vtkPVRenderView* renderView = vtkPVRenderView::SafeDownCast(proxy->GetClientSideObject());
        if (renderView)
        {
          // And the local renderer must be of type vtkOpenGLRenderer
          oglRen = vtkOpenGLRenderer::SafeDownCast(renderView->GetRenderer());
          if (!oglRen)
          {
            vtkGenericWarningMacro("Cannot get vtkOpenGLRenderer (client side renderer "
              << "is of type " << renderView->GetRenderer()->GetClassName()
              << ", expected a vtkOpenGLRenderer)");
          }
        }
        else
        {
          vtkGenericWarningMacro(<< "Cannot get vtkOpenGLRenderer (view proxy client side object"
                                 << " is of type " << proxy->GetClientSideObject()->GetClassName()
                                 << ", expected a vtkPVRenderView)");
        }
      }
      else
      {
        vtkGenericWarningMacro(<< "Cannot get vtkOpenGLRenderer (view proxy is of type "
                               << view->getViewProxy()->GetClassName()
                               << ", expected a vtkSMRenderViewProxy)");
      }
    }
    else
    {
      vtkGenericWarningMacro(<< "Cannot get vtkOpenGLRenderer (pqView is null)");
    }
    return oglRen;
  }

  /*
   * Lazily initialize internal data structures and connect to the collaboration
   * server.
   */
  void InitializeCollaboration(pqView* view)
  {
    if (!this->CollabEnabled)
    {
      return;
    }

    this->ActiveView = view;

    vtkOpenGLRenderer* oglRen = this->GetOpenGLRenderer(view);
    if (!oglRen)
    {
      vtkGenericWarningMacro("Cannot inititialize collaboration");
      return;
    }

    if (!this->CollaborationClient)
    {
      this->CollaborationClient = vtkSmartPointer<vtkVRCollaborationClient>::New();
      this->CollaborationClient->SetLogCallback(
        std::bind(&pqVRCollaborationWidget::pqInternals::collaborationLoggingCallback, this,
          std::placeholders::_1, std::placeholders::_2));
    }

    // Now that we have the renderer and collab client, connect
    this->CollaborationClient->SetCollabHost(this->CollabHost.toUtf8().constData());
    this->CollaborationClient->SetCollabPort(this->CollabPort);
    this->CollaborationClient->SetCollabSession(this->CollabSession.toUtf8().constData());
    this->CollaborationClient->SetCollabName(this->CollabName.toUtf8().constData());
    this->CollaborationClient->SetAvatarInitialUpVector(
      this->CollabAvatarUpX, this->CollabAvatarUpY, this->CollabAvatarUpZ);

    // This interactor style proxy enables "outgoing" collaboration
    if (!this->CollabStyleProxy)
    {
      this->CollabStyleProxy = vtkSmartPointer<vtkSMVRCollaborationStyleProxy>::New();
      this->CollabStyleProxy->IsInternalOn();
      this->CollabStyleProxy->SetCollaborationClient(this->CollaborationClient);
    }

    // Connect the INTERACTOR_STYLE_NAVIGATION events on the render view proxy
    // to the observer in the proxy class responsible for sharing navigation info
    // with collaborators.
    vtkSMRenderViewProxy* proxy = vtkSMRenderViewProxy::SafeDownCast(view->getViewProxy());
    if (proxy && this->CollabStyleProxy->GetNavigationObserver())
    {
      proxy->AddObserver(vtkSMVRInteractorStyleProxy::INTERACTOR_STYLE_NAVIGATION,
        this->CollabStyleProxy->GetNavigationObserver());
    }

    this->UpdateCollaborationStyleProxy();

    vtkSMVRCollaborationStyleProxy* foundIt = this->FindCollaborationProxyInHandler();

    if (!foundIt)
    {
      pqVRQueueHandler::instance()->add(this->CollabStyleProxy.Get());
    }

    this->CollaborationClient->SetMoveEventSource(this->CollabStyleProxy.Get());
    this->CollaborationClient->Initialize(oglRen);

    if (!this->StartRenderObserver)
    {
      this->StartRenderObserver = vtkSmartPointer<vtkStartRenderObserver>::New();
      this->StartRenderObserver->SetCollaborationClient(this->CollaborationClient);
    }

    // Use the start render event as a hook for collaborationClient->Render()
    oglRen->AddObserver(vtkCommand::StartEvent, this->StartRenderObserver);
  }

  /*
   * Update the vtkSMVRCollaborationStyleProxy with locally stored configuration
   * details.
   */
  void UpdateCollaborationStyleProxy()
  {
    if (this->CollabStyleProxy)
    {
      this->CollabStyleProxy->SetHeadEventName(&(this->HeadEventName));
      this->CollabStyleProxy->SetLeftHandEventName(&(this->LeftHandEventName));
      this->CollabStyleProxy->SetRightHandEventName(&(this->RightHandEventName));
      this->CollabStyleProxy->SetNavigationSharing(this->NavigationSharingEnabled);
    }
  }

  /*
   * We manage a single instance of this internal interactor style proxy, check
   * if we have added that instance to the queue handler.  Return it if we have
   * it, or a nullptr otherwise.
   */
  vtkSMVRCollaborationStyleProxy* FindCollaborationProxyInHandler()
  {
    if (!this->CollabStyleProxy)
    {
      // We haven't even created it yet
      return nullptr;
    }

    vtkSMVRCollaborationStyleProxy* foundIt = nullptr;

    Q_FOREACH (vtkSMVRInteractorStyleProxy* style, pqVRQueueHandler::instance()->styles())
    {
      if (style == this->CollabStyleProxy.Get())
      {
        foundIt = vtkSMVRCollaborationStyleProxy::SafeDownCast(style);
        break;
      }
    }

    return foundIt;
  }

  /*
   * If we are connected, disconnect and clean up, leaving the state
   * ready to connect again.
   */
  void StopCollaboration()
  {
    if (!this->CollabEnabled || !this->CollaborationClient)
    {
      return;
    }

    this->CollaborationClient->Disconnect();

    vtkOpenGLRenderer* oglRen = this->GetOpenGLRenderer(this->ActiveView);
    if (!oglRen)
    {
      vtkGenericWarningMacro("Cannot remove collaboration renderer observer");
      return;
    }

    if (this->StartRenderObserver)
    {
      oglRen->RemoveObserver(this->StartRenderObserver);
    }

    if (this->CollabStyleProxy->GetNavigationObserver())
    {
      vtkSMRenderViewProxy* proxy =
        vtkSMRenderViewProxy::SafeDownCast(this->ActiveView->getViewProxy());
      if (proxy)
      {
        proxy->RemoveObserver(this->CollabStyleProxy->GetNavigationObserver());
      }
    }

    vtkSMVRCollaborationStyleProxy* foundIt = this->FindCollaborationProxyInHandler();
    if (foundIt)
    {
      pqVRQueueHandler::instance()->remove(foundIt);
    }
  }

  /* Sample PVVR <VRCollaboration> configuration:

      <VRCollaboration>
        <Enabled value="true"/>
        <Host value="vrserver.kitware.com"/>
        <Port value="5555"/>
        <Session value="pv"/>
        <Name value="Anonymous"/>
        <AvatarUpX value="0"/>
        <AvatarUpY value="1"/>
        <AvatarUpZ value="0"/>
        <AvatarEvents>
          <Head value="vrconn.head"/>
          <LeftHand value="vrconn.wandl"/>
          <RightHand value="vrconn.wandr"/>
        </AvatarEvents>
      </VRCollaboration>

  */

  /*
   * Restore widget/connection information from state
   */
  void restoreCollaborationState(vtkPVXMLElement* xml)
  {
    if (!xml)
    {
      return;
    }

    vtkPVXMLElement* sectionParent = xml->FindNestedElementByName("VRCollaboration");
    if (!sectionParent)
    {
      return;
    }

    vtkPVXMLElement* child = sectionParent->FindNestedElementByName("Enabled");
    if (child)
    {
      const char* attr = child->GetAttributeOrEmpty("value");
      this->CollabEnabled = strcmp(attr, "true") == 0 ? true : false;
    }

    child = sectionParent->FindNestedElementByName("Host");
    if (child)
    {
      this->CollabHost = child->GetAttributeOrEmpty("value");
    }

    child = sectionParent->FindNestedElementByName("Port");
    if (child)
    {
      const char* portstr = child->GetAttributeOrEmpty("value");
      if (strcmp(portstr, "") != 0)
      {
        this->CollabPort = atoi(portstr);
      }
    }

    child = sectionParent->FindNestedElementByName("Session");
    if (child)
    {
      this->CollabSession = child->GetAttributeOrEmpty("value");
    }

    child = sectionParent->FindNestedElementByName("Name");
    if (child)
    {
      this->CollabName = child->GetAttributeOrEmpty("value");
    }

    child = sectionParent->FindNestedElementByName("AvatarUpX");
    if (child)
    {
      const char* upxstr = child->GetAttributeOrEmpty("value");
      if (strcmp(upxstr, "") != 0)
      {
        this->CollabAvatarUpX = atof(upxstr);
      }
    }

    child = sectionParent->FindNestedElementByName("AvatarUpY");
    if (child)
    {
      const char* upystr = child->GetAttributeOrEmpty("value");
      if (strcmp(upystr, "") != 0)
      {
        this->CollabAvatarUpY = atof(upystr);
      }
    }

    child = sectionParent->FindNestedElementByName("AvatarUpZ");
    if (child)
    {
      const char* upzstr = child->GetAttributeOrEmpty("value");
      if (strcmp(upzstr, "") != 0)
      {
        this->CollabAvatarUpZ = atof(upzstr);
      }
    }

    child = sectionParent->FindNestedElementByName("AvatarEvents");

    if (child)
    {
      vtkPVXMLElement* grandChild = child->FindNestedElementByName("Head");
      if (grandChild)
      {
        this->HeadEventName = grandChild->GetAttributeOrEmpty("value");
      }

      grandChild = child->FindNestedElementByName("LeftHand");
      if (grandChild)
      {
        this->LeftHandEventName = grandChild->GetAttributeOrEmpty("value");
      }

      grandChild = child->FindNestedElementByName("RightHand");
      if (grandChild)
      {
        this->RightHandEventName = grandChild->GetAttributeOrEmpty("value");
      }

      grandChild = child->FindNestedElementByName("ShareNavigation");
      if (grandChild)
      {
        const char* attr = grandChild->GetAttributeOrEmpty("value");
        this->NavigationSharingEnabled = strcmp(attr, "true") == 0 ? true : false;
      }
    }
  }

  /*
   * Save widget/connection information to state
   */
  void saveCollaborationState(vtkPVXMLElement* root)
  {
    if (!root)
    {
      return;
    }

    vtkPVXMLElement* sectionParent = vtkPVXMLElement::New();
    sectionParent->SetName("VRCollaboration");

    vtkPVXMLElement* child = vtkPVXMLElement::New();
    child->SetName("Enabled");
    child->AddAttribute("value", (this->CollabEnabled ? "true" : "false"));
    sectionParent->AddNestedElement(child);
    child->Delete();

    child = vtkPVXMLElement::New();
    child->SetName("Host");
    child->AddAttribute("value", this->CollabHost.toUtf8().constData());
    sectionParent->AddNestedElement(child);
    child->Delete();

    child = vtkPVXMLElement::New();
    child->SetName("Port");
    child->AddAttribute("value", this->CollabPort);
    sectionParent->AddNestedElement(child);
    child->Delete();

    child = vtkPVXMLElement::New();
    child->SetName("Session");
    child->AddAttribute("value", this->CollabSession.toUtf8().constData());
    sectionParent->AddNestedElement(child);
    child->Delete();

    child = vtkPVXMLElement::New();
    child->SetName("Name");
    child->AddAttribute("value", this->CollabName.toUtf8().constData());
    sectionParent->AddNestedElement(child);
    child->Delete();

    child = vtkPVXMLElement::New();
    child->SetName("AvatarUpX");
    child->AddAttribute("value", this->CollabAvatarUpX);
    sectionParent->AddNestedElement(child);
    child->Delete();

    child = vtkPVXMLElement::New();
    child->SetName("AvatarUpY");
    child->AddAttribute("value", this->CollabAvatarUpY);
    sectionParent->AddNestedElement(child);
    child->Delete();

    child = vtkPVXMLElement::New();
    child->SetName("AvatarUpZ");
    child->AddAttribute("value", this->CollabAvatarUpZ);
    sectionParent->AddNestedElement(child);
    child->Delete();

    child = vtkPVXMLElement::New();
    child->SetName("AvatarEvents");

    vtkPVXMLElement* grandChild = vtkPVXMLElement::New();
    grandChild->SetName("Head");
    grandChild->AddAttribute("value", this->HeadEventName.toUtf8().constData());
    child->AddNestedElement(grandChild);
    grandChild->Delete();

    grandChild = vtkPVXMLElement::New();
    grandChild->SetName("LeftHand");
    grandChild->AddAttribute("value", this->LeftHandEventName.toUtf8().constData());
    child->AddNestedElement(grandChild);
    grandChild->Delete();

    grandChild = vtkPVXMLElement::New();
    grandChild->SetName("RightHand");
    grandChild->AddAttribute("value", this->RightHandEventName.toUtf8().constData());
    child->AddNestedElement(grandChild);
    grandChild->Delete();

    grandChild = vtkPVXMLElement::New();
    grandChild->SetName("ShareNavigation");
    grandChild->AddAttribute("value", (this->NavigationSharingEnabled ? "true" : "false"));
    child->AddNestedElement(grandChild);
    grandChild->Delete();

    sectionParent->AddNestedElement(child);
    child->Delete();

    root->AddNestedElement(sectionParent);
    sectionParent->Delete();
  }

  pqView* ActiveView;
  pqVRAvatarEvents* AvatarConfigDialog;
  vtkSmartPointer<vtkVRCollaborationClient> CollaborationClient;
  vtkSmartPointer<vtkStartRenderObserver> StartRenderObserver;
  vtkSmartPointer<vtkSMVRCollaborationStyleProxy> CollabStyleProxy;
  bool CollabEnabled;
  QString CollabHost;
  QString CollabSession;
  QString CollabName;
  int CollabPort;
  double CollabAvatarUpX;
  double CollabAvatarUpY;
  double CollabAvatarUpZ;
  QString HeadEventName;
  QString LeftHandEventName;
  QString RightHandEventName;
  bool NavigationSharingEnabled;

  QStringList TrackerNames;
};

//-----------------------------------------------------------------------------
pqVRCollaborationWidget::pqVRCollaborationWidget(QWidget* parentObject, Qt::WindowFlags f)
  : Superclass(parentObject, f)
  , Internals(new pqInternals())
{
  this->Internals->setupUi(this);

  this->Internals->AvatarConfigDialog = new pqVRAvatarEvents(this);
  this->Internals->CollaborationClient = nullptr;
  this->Internals->StartRenderObserver = nullptr;
  this->Internals->CollabStyleProxy = nullptr;
  this->Internals->CollabEnabled = false;
  this->Internals->NavigationSharingEnabled = false;
  this->updateCollabWidgetState();

  this->Internals->outputWindow->setReadOnly(true);

  connect(this->Internals->configureAvatarBtn, SIGNAL(clicked()), this, SLOT(configureAvatar()));

  QObject::connect(this->Internals->cCollabEnable, &QCheckBox::pqCheckBoxSignal, this,
    [&](pqCheckState state) { this->collabEnabledChanged(static_cast<Qt::CheckState>(state)); });

  connect(this->Internals->cPortValue, SIGNAL(editingFinished()), this, SLOT(collabPortChanged()));

  connect(
    this->Internals->cServerValue, SIGNAL(editingFinished()), this, SLOT(collabServerChanged()));

  connect(
    this->Internals->cSessionValue, SIGNAL(editingFinished()), this, SLOT(collabSessionChanged()));

  connect(this->Internals->cNameValue, SIGNAL(editingFinished()), this, SLOT(collabNameChanged()));

  connect(
    this->Internals->cAvatarUpX, SIGNAL(editingFinished()), this, SLOT(collabAvatarUpXChanged()));

  connect(
    this->Internals->cAvatarUpY, SIGNAL(editingFinished()), this, SLOT(collabAvatarUpYChanged()));

  connect(
    this->Internals->cAvatarUpZ, SIGNAL(editingFinished()), this, SLOT(collabAvatarUpZChanged()));

  connect(pqApplicationCore::instance(), SIGNAL(stateSaved(vtkPVXMLElement*)), this,
    SLOT(saveCollaborationState(vtkPVXMLElement*)));

  connect(pqApplicationCore::instance(), SIGNAL(stateLoaded(vtkPVXMLElement*, vtkSMProxyLocator*)),
    this, SLOT(restoreCollaborationState(vtkPVXMLElement*, vtkSMProxyLocator*)));

  // Make sure all collaboration settings are updated with default values
  this->collabPortChanged();
  this->collabServerChanged();
  this->collabSessionChanged();
  this->collabNameChanged();
  this->collabAvatarUpXChanged();
  this->collabAvatarUpYChanged();
  this->collabAvatarUpZChanged();
}

//-----------------------------------------------------------------------------
pqVRCollaborationWidget::~pqVRCollaborationWidget()
{
  if (this->Internals->AvatarConfigDialog != nullptr)
  {
    delete this->Internals->AvatarConfigDialog;
  }

  delete this->Internals;
}

//-----------------------------------------------------------------------------
void pqVRCollaborationWidget::initializeCollaboration(pqView* view)
{
  this->Internals->InitializeCollaboration(view);
}

//-----------------------------------------------------------------------------
void pqVRCollaborationWidget::stopCollaboration()
{
  this->Internals->StopCollaboration();
}

//-----------------------------------------------------------------------------
void pqVRCollaborationWidget::configureAvatar()
{
  pqVRAvatarEvents* dialog = this->Internals->AvatarConfigDialog;

  if (dialog->exec() == QDialog::Accepted)
  {
    dialog->getEventName(pqVRAvatarEvents::Head, this->Internals->HeadEventName);
    dialog->getEventName(pqVRAvatarEvents::LeftHand, this->Internals->LeftHandEventName);
    dialog->getEventName(pqVRAvatarEvents::RightHand, this->Internals->RightHandEventName);
    this->Internals->NavigationSharingEnabled = dialog->getNavigationSharing();
    this->Internals->UpdateCollaborationStyleProxy();
  }
}

//-----------------------------------------------------------------------------
void pqVRCollaborationWidget::updateCollabWidgetState()
{
  bool enabled = this->Internals->CollabEnabled;
  this->Internals->configureAvatarBtn->setEnabled(enabled);
  this->Internals->cServerValue->setEnabled(enabled);
  this->Internals->cSessionValue->setEnabled(enabled);
  this->Internals->cNameValue->setEnabled(enabled);
  this->Internals->cPortValue->setEnabled(enabled);
  this->Internals->cAvatarUpX->setEnabled(enabled);
  this->Internals->cAvatarUpY->setEnabled(enabled);
  this->Internals->cAvatarUpZ->setEnabled(enabled);
}

//-----------------------------------------------------------------------------
void pqVRCollaborationWidget::collabEnabledChanged(Qt::CheckState state)
{
  this->Internals->CollabEnabled = state != 0;
  this->updateCollabWidgetState();
}

//-----------------------------------------------------------------------------
void pqVRCollaborationWidget::collabServerChanged()
{
  this->Internals->CollabHost = this->Internals->cServerValue->text();
}

//-----------------------------------------------------------------------------
void pqVRCollaborationWidget::collabSessionChanged()
{
  this->Internals->CollabSession = this->Internals->cSessionValue->text();
}

//-----------------------------------------------------------------------------
void pqVRCollaborationWidget::collabNameChanged()
{
  this->Internals->CollabName = this->Internals->cNameValue->text();
}

//-----------------------------------------------------------------------------
void pqVRCollaborationWidget::collabPortChanged()
{
  int port = this->Internals->cPortValue->text().toInt();
  this->Internals->CollabPort = port;
}

//-----------------------------------------------------------------------------
void pqVRCollaborationWidget::collabAvatarUpXChanged()
{
  double avUpX = this->Internals->cAvatarUpX->text().toDouble();
  this->Internals->CollabAvatarUpX = avUpX;
}

//-----------------------------------------------------------------------------
void pqVRCollaborationWidget::collabAvatarUpYChanged()
{
  double avUpY = this->Internals->cAvatarUpY->text().toDouble();
  this->Internals->CollabAvatarUpY = avUpY;
}

//-----------------------------------------------------------------------------
void pqVRCollaborationWidget::collabAvatarUpZChanged()
{
  double avUpZ = this->Internals->cAvatarUpZ->text().toDouble();
  this->Internals->CollabAvatarUpZ = avUpZ;
}

//-----------------------------------------------------------------------------
void pqVRCollaborationWidget::saveCollaborationState(vtkPVXMLElement* root)
{
  this->Internals->saveCollaborationState(root);
}

//-----------------------------------------------------------------------------
void pqVRCollaborationWidget::restoreCollaborationState(
  vtkPVXMLElement* root, vtkSMProxyLocator* locator)
{
  this->Internals->restoreCollaborationState(root);

  this->Internals->cCollabEnable->setChecked(this->Internals->CollabEnabled);
  this->Internals->cServerValue->setText(this->Internals->CollabHost);
  this->Internals->cPortValue->setText(QString::number(this->Internals->CollabPort));
  this->Internals->cSessionValue->setText(this->Internals->CollabSession);
  this->Internals->cNameValue->setText(this->Internals->CollabName);
  this->Internals->cAvatarUpX->setText(QString::number(this->Internals->CollabAvatarUpX));
  this->Internals->cAvatarUpY->setText(QString::number(this->Internals->CollabAvatarUpY));
  this->Internals->cAvatarUpZ->setText(QString::number(this->Internals->CollabAvatarUpZ));

  pqVRAvatarEvents* dialog = this->Internals->AvatarConfigDialog;
  dialog->setEventName(pqVRAvatarEvents::Head, this->Internals->HeadEventName);
  dialog->setEventName(pqVRAvatarEvents::LeftHand, this->Internals->LeftHandEventName);
  dialog->setEventName(pqVRAvatarEvents::RightHand, this->Internals->RightHandEventName);
  dialog->setNavigationSharing(this->Internals->NavigationSharingEnabled);
}
#else
pqVRCollaborationWidget::pqVRCollaborationWidget(QWidget* parentObject, Qt::WindowFlags f) {}
pqVRCollaborationWidget::~pqVRCollaborationWidget() {}
void pqVRCollaborationWidget::initializeCollaboration(pqView* view) {}
void pqVRCollaborationWidget::stopCollaboration() {}
void pqVRCollaborationWidget::configureAvatar() {}
void pqVRCollaborationWidget::updateCollabWidgetState() {}
void pqVRCollaborationWidget::collabEnabledChanged(Qt::CheckState state) {}
void pqVRCollaborationWidget::collabServerChanged() {}
void pqVRCollaborationWidget::collabSessionChanged() {}
void pqVRCollaborationWidget::collabNameChanged() {}
void pqVRCollaborationWidget::collabPortChanged() {}
void pqVRCollaborationWidget::collabAvatarUpXChanged() {}
void pqVRCollaborationWidget::collabAvatarUpYChanged() {}
void pqVRCollaborationWidget::collabAvatarUpZChanged() {}
void pqVRCollaborationWidget::saveCollaborationState(vtkPVXMLElement* root) {}
void pqVRCollaborationWidget::restoreCollaborationState(
  vtkPVXMLElement* root, vtkSMProxyLocator* locator)
{
}
#endif
