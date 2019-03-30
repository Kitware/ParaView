/*=========================================================================

   Program: ParaView
   Module:  pqPropertiesPanel.cxx

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
#include "pqPropertiesPanel.h"
#include "ui_pqPropertiesPanel.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqDataRepresentation.h"
#include "pqLiveInsituManager.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqProxyWidget.h"
#include "pqSearchBox.h"
#include "pqServerManagerModel.h"
#include "pqSettings.h"
#include "pqTimer.h"
#include "pqUndoStack.h"
#include "vtkCollection.h"
#include "vtkCommand.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkNew.h"
#include "vtkPVGeneralSettings.h"
#include "vtkPVLogger.h"
#include "vtkSMProperty.h"
#include "vtkSMProxyClipboard.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMViewProxy.h"
#include "vtkTimerLog.h"

#include <QKeyEvent>
#include <QPointer>
#include <QStyle>
#include <QStyleFactory>

#include <cassert>
#include <iostream>

namespace
{

// internal class used to keep track of all the widgets associated with a
// panel either for a source or representation.
class pqProxyWidgets : public QObject
{
  static unsigned long Counter;
  unsigned long MyId;

public:
  typedef QObject Superclass;
  QPointer<pqProxy> Proxy;
  QPointer<pqProxyWidget> Panel;
  pqProxyWidgets(pqProxy* proxy, pqPropertiesPanel* panel)
    : Superclass(panel)
  {
    this->MyId = pqProxyWidgets::Counter++;

    this->Proxy = proxy;
    this->Panel = new pqProxyWidget(proxy->getProxy());
    this->Panel->setObjectName(QString("HiddenProxyPanel%1").arg(this->MyId));
    this->Panel->setView(panel->view());
    QObject::connect(panel, SIGNAL(viewChanged(pqView*)), this->Panel, SLOT(setView(pqView*)));
  }

  ~pqProxyWidgets() override { delete this->Panel; }

  void hide()
  {
    this->Panel->setObjectName(QString("HiddenProxyPanel%1").arg(this->MyId));
    this->Panel->hide();
    this->Panel->parentWidget()->layout()->removeWidget(this->Panel);
  }

  void show(QWidget* parentWdg)
  {
    assert(parentWdg != NULL);

    delete parentWdg->layout();
    QVBoxLayout* layout = new QVBoxLayout(parentWdg);
    layout->setMargin(0);
    layout->setSpacing(0);
    this->Panel->setObjectName("ProxyPanel");
    this->Panel->setParent(parentWdg);
    layout->addWidget(this->Panel);
    this->Panel->show();
  }

  void showWidgets(bool show_advanced, const QString& filterText)
  {
    this->Panel->filterWidgets(show_advanced, filterText);
  }

  void apply(pqView* vtkNotUsed(view))
  {
    vtkVLogIfF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), this->Proxy != nullptr,
      "applying changes to `%s`", this->Proxy->getProxy()->GetLogNameOrDefault());
    this->Panel->apply();
  }

  void reset()
  {
    this->Panel->reset();

    // this ensures that we don't change the state of UNINITIALIZED proxies.
    if (this->Proxy->modifiedState() == pqProxy::MODIFIED)
    {
      this->Proxy->setModifiedState(pqProxy::UNMODIFIED);
    }
  }

private:
  Q_DISABLE_COPY(pqProxyWidgets)
};

unsigned long pqProxyWidgets::Counter = 0;
};

bool pqPropertiesPanel::AutoApply = false;
int pqPropertiesPanel::AutoApplyDelay = 0; // in msec

//*****************************************************************************
class pqPropertiesPanel::pqInternals
{
public:
  Ui::propertiesPanel Ui;
  QPointer<pqView> View;
  QPointer<pqOutputPort> Port;
  QPointer<pqPipelineSource> Source;
  QPointer<pqDataRepresentation> Representation;
  QMap<void*, QPointer<pqProxyWidgets> > SourceWidgets;
  QPointer<pqProxyWidgets> DisplayWidgets;
  QPointer<pqProxyWidgets> ViewWidgets;
  bool ReceivedChangeAvailable;
  pqTimer AutoApplyTimer;
  vtkNew<vtkSMProxyClipboard> SourceClipboard;
  vtkNew<vtkSMProxyClipboard> DisplayClipboard;
  vtkNew<vtkSMProxyClipboard> ViewClipboard;

  vtkNew<vtkEventQtSlotConnect> RepresentationEventConnect;

  //---------------------------------------------------------------------------
  pqInternals(pqPropertiesPanel* panel)
    : ReceivedChangeAvailable(false)
  {
    this->Ui.setupUi(panel);

    // Setup background color for Apply button so users notice it when it's
    // enabled.

    // if XP Style is being used swap it out for cleanlooks which looks
    // almost the same so we can have a green apply button make all
    // the buttons the same
    QStyle* styleLocal = this->Ui.Accept->style();
    QString styleName = styleLocal->metaObject()->className();
    if (styleName == "QWindowsXPStyle")
    {
      styleLocal = QStyleFactory::create("cleanlooks");
      styleLocal->setParent(panel);
      this->Ui.Accept->setStyle(styleLocal);
      this->Ui.Reset->setStyle(styleLocal);
      this->Ui.Delete->setStyle(styleLocal);
      this->Ui.PropertiesRestoreDefaults->setStyle(styleLocal);
      this->Ui.PropertiesSaveAsDefaults->setStyle(styleLocal);
      this->Ui.DisplayRestoreDefaults->setStyle(styleLocal);
      this->Ui.DisplaySaveAsDefaults->setStyle(styleLocal);
      this->Ui.ViewRestoreDefaults->setStyle(styleLocal);
      this->Ui.DisplaySaveAsDefaults->setStyle(styleLocal);
      QPalette buttonPalette = this->Ui.Accept->palette();
      buttonPalette.setColor(QPalette::Button, QColor(244, 246, 244));
      this->Ui.Accept->setPalette(buttonPalette);
      this->Ui.Reset->setPalette(buttonPalette);
      this->Ui.Delete->setPalette(buttonPalette);
    }

    // Add icons to the settings save/restore defaults buttons
    this->Ui.PropertiesRestoreDefaults->setIcon(styleLocal->standardIcon(QStyle::SP_BrowserReload));
    this->Ui.PropertiesSaveAsDefaults->setIcon(
      styleLocal->standardIcon(QStyle::SP_DialogSaveButton));
    this->Ui.DisplayRestoreDefaults->setIcon(styleLocal->standardIcon(QStyle::SP_BrowserReload));
    this->Ui.DisplaySaveAsDefaults->setIcon(styleLocal->standardIcon(QStyle::SP_DialogSaveButton));
    this->Ui.ViewRestoreDefaults->setIcon(styleLocal->standardIcon(QStyle::SP_BrowserReload));
    this->Ui.ViewSaveAsDefaults->setIcon(styleLocal->standardIcon(QStyle::SP_DialogSaveButton));

    this->Ui.PropertiesButtons->layout()->setSpacing(
      pqPropertiesPanel::suggestedHorizontalSpacing());
    this->Ui.DisplayButtons->layout()->setSpacing(pqPropertiesPanel::suggestedHorizontalSpacing());
    this->Ui.ViewButtons->layout()->setSpacing(pqPropertiesPanel::suggestedHorizontalSpacing());

    // change the apply button palette so it is green when it is enabled.
    QPalette applyPalette = this->Ui.Accept->palette();
    applyPalette.setColor(QPalette::Active, QPalette::Button, QColor(161, 213, 135));
    applyPalette.setColor(QPalette::Inactive, QPalette::Button, QColor(161, 213, 135));
    this->Ui.Accept->setPalette(applyPalette);

    this->AutoApplyTimer.setSingleShot(true);
    panel->connect(&this->AutoApplyTimer, SIGNAL(timeout()), SLOT(apply()));
  }

  ~pqInternals()
  {
    foreach (pqProxyWidgets* widgets, this->SourceWidgets)
    {
      delete widgets;
    }
    this->SourceWidgets.clear();
    delete this->DisplayWidgets;
    delete this->ViewWidgets;
  }

  void triggerAutoApply() { this->AutoApplyTimer.start(pqPropertiesPanel::autoApplyDelay()); }

  //---------------------------------------------------------------------------
  void updateInformationAndDomains()
  {
    if (this->Source)
    {
      // this->Source->updatePipeline();
      vtkSMProxy* proxy = this->Source->getProxy();
      if (vtkSMSourceProxy* sourceProxy = vtkSMSourceProxy::SafeDownCast(proxy))
      {
        sourceProxy->UpdatePipelineInformation();
      }
      else
      {
        proxy->UpdatePropertyInformation();
      }
    }
  }
};

//-----------------------------------------------------------------------------
pqPropertiesPanel::pqPropertiesPanel(QWidget* parentObject)
  : Superclass(parentObject)
  , Internals(new pqInternals(this))
  , PanelMode(pqPropertiesPanel::ALL_PROPERTIES)
{
  // Get AutoApply setting from GeneralSettings object.
  vtkPVGeneralSettings* generalSettings = vtkPVGeneralSettings::GetInstance();
  pqPropertiesPanel::AutoApply = generalSettings->GetAutoApply();

  // every time the settings change, we need to update our auto-apply status.
  pqCoreUtilities::connect(
    generalSettings, vtkCommand::ModifiedEvent, this, SLOT(generalSettingsChanged()));

  //---------------------------------------------------------------------------
  // Listen to various signals from the application indicating changes in active
  // source/view/representation, etc.
  pqActiveObjects* activeObjects = &pqActiveObjects::instance();
  this->connect(
    activeObjects, SIGNAL(portChanged(pqOutputPort*)), this, SLOT(setOutputPort(pqOutputPort*)));
  this->connect(activeObjects, SIGNAL(viewChanged(pqView*)), this, SLOT(setView(pqView*)));
  this->connect(activeObjects, SIGNAL(representationChanged(pqDataRepresentation*)), this,
    SLOT(setRepresentation(pqDataRepresentation*)));

  // listen to server manager changes
  pqServerManagerModel* smm = pqApplicationCore::instance()->getServerManagerModel();
  this->connect(
    smm, SIGNAL(sourceRemoved(pqPipelineSource*)), SLOT(proxyDeleted(pqPipelineSource*)));
  // this connection ensures that the button state is updated everytime any
  // item's state changes.
  this->connect(
    smm, SIGNAL(modifiedStateChanged(pqServerManagerModelItem*)), SLOT(updateButtonState()));
  this->connect(pqApplicationCore::instance()->getServerManagerModel(),
    SIGNAL(connectionRemoved(pqPipelineSource*, pqPipelineSource*, int)),
    SLOT(updateButtonState()));
  this->connect(pqApplicationCore::instance()->getServerManagerModel(),
    SIGNAL(connectionAdded(pqPipelineSource*, pqPipelineSource*, int)), SLOT(updateButtonState()));

  // Listen to UI signals.
  QObject::connect(this->Internals->Ui.Accept, SIGNAL(clicked()), this, SLOT(apply()));
  QObject::connect(this->Internals->Ui.Reset, SIGNAL(clicked()), this, SLOT(reset()));
  QObject::connect(this->Internals->Ui.Delete, SIGNAL(clicked()), this, SLOT(deleteProxy()));
  QObject::connect(this->Internals->Ui.Help, SIGNAL(clicked()), this, SLOT(showHelp()));
  QObject::connect(
    this->Internals->Ui.SearchBox, SIGNAL(textChanged(QString)), this, SLOT(updatePanel()));
  QObject::connect(this->Internals->Ui.PropertiesRestoreDefaults, SIGNAL(clicked()), this,
    SLOT(propertiesRestoreDefaults()));
  QObject::connect(this->Internals->Ui.PropertiesSaveAsDefaults, SIGNAL(clicked()), this,
    SLOT(propertiesSaveAsDefaults()));
  QObject::connect(this->Internals->Ui.SearchBox, SIGNAL(advancedSearchActivated(bool)), this,
    SLOT(updatePanel()));

  QObject::connect(this->Internals->Ui.PropertiesButton, SIGNAL(toggled(bool)),
    this->Internals->Ui.PropertiesFrame, SLOT(setVisible(bool)));
  QObject::connect(this->Internals->Ui.DisplayRestoreDefaults, SIGNAL(clicked()), this,
    SLOT(displayRestoreDefaults()));
  QObject::connect(this->Internals->Ui.DisplaySaveAsDefaults, SIGNAL(clicked()), this,
    SLOT(displaySaveAsDefaults()));
  QObject::connect(
    this->Internals->Ui.ViewRestoreDefaults, SIGNAL(clicked()), this, SLOT(viewRestoreDefaults()));
  QObject::connect(
    this->Internals->Ui.ViewSaveAsDefaults, SIGNAL(clicked()), this, SLOT(viewSaveAsDefaults()));
  QObject::connect(this->Internals->Ui.DisplayButton, SIGNAL(toggled(bool)),
    this->Internals->Ui.DisplayFrame, SLOT(setVisible(bool)));
  QObject::connect(this->Internals->Ui.ViewButton, SIGNAL(toggled(bool)),
    this->Internals->Ui.ViewFrame, SLOT(setVisible(bool)));

  this->connect(this->Internals->Ui.PropertiesCopy, SIGNAL(clicked()), SLOT(copyProperties()));
  this->connect(this->Internals->Ui.PropertiesPaste, SIGNAL(clicked()), SLOT(pasteProperties()));
  this->connect(this->Internals->Ui.DisplayCopy, SIGNAL(clicked()), SLOT(copyDisplay()));
  this->connect(this->Internals->Ui.DisplayPaste, SIGNAL(clicked()), SLOT(pasteDisplay()));
  this->connect(this->Internals->Ui.ViewCopy, SIGNAL(clicked()), SLOT(copyView()));
  this->connect(this->Internals->Ui.ViewPaste, SIGNAL(clicked()), SLOT(pasteView()));

  this->setOutputPort(NULL);
}

//-----------------------------------------------------------------------------
pqPropertiesPanel::~pqPropertiesPanel()
{
  delete this->Internals;
  this->Internals = 0;
}

//-----------------------------------------------------------------------------
bool pqPropertiesPanel::canApply()
{
  Ui::propertiesPanel& ui = this->Internals->Ui;
  return ui.Accept->isEnabled();
}

//-----------------------------------------------------------------------------
bool pqPropertiesPanel::canReset()
{
  Ui::propertiesPanel& ui = this->Internals->Ui;
  return ui.Reset->isEnabled();
}

//-----------------------------------------------------------------------------
void pqPropertiesPanel::setPanelMode(int val)
{
  if (this->PanelMode == val)
  {
    return;
  }

  this->PanelMode = val;

  // show buttons only when showing source properties.
  bool has_source = (this->PanelMode & pqPropertiesPanel::SOURCE_PROPERTIES) != 0;
  bool has_display = (this->PanelMode & pqPropertiesPanel::DISPLAY_PROPERTIES) != 0;
  bool has_view = (this->PanelMode & pqPropertiesPanel::VIEW_PROPERTIES) != 0;

  this->Internals->Ui.Accept->setVisible(has_source);
  this->Internals->Ui.Delete->setVisible(has_source);
  this->Internals->Ui.Help->setVisible(has_source);
  this->Internals->Ui.Reset->setVisible(has_source);

  this->Internals->Ui.PropertiesButtons->setVisible(has_source);
  this->Internals->Ui.PropertiesFrame->setVisible(has_source);

  this->Internals->Ui.ViewFrame->setVisible(has_view);
  this->Internals->Ui.ViewButtons->setVisible(has_view);

  this->Internals->Ui.DisplayFrame->setVisible(has_display);
  this->Internals->Ui.DisplayButtons->setVisible(has_display);

  // the buttons need not be shown if there's only 1 type in the panel.
  bool has_multiples_types =
    ((has_source ? 1 : 0) + (has_display ? 1 : 0) + (has_view ? 1 : 0)) > 1;
  this->Internals->Ui.PropertiesButton->setVisible(has_multiples_types && has_source);
  this->Internals->Ui.DisplayButton->setVisible(has_multiples_types && has_display);
  this->Internals->Ui.ViewButton->setVisible(has_multiples_types && has_view);

  this->updatePanel();
}

//-----------------------------------------------------------------------------
void pqPropertiesPanel::generalSettingsChanged()
{
  vtkPVGeneralSettings* generalSettings = vtkPVGeneralSettings::GetInstance();
  pqPropertiesPanel::AutoApply = generalSettings->GetAutoApply();
}

//-----------------------------------------------------------------------------
void pqPropertiesPanel::setAutoApply(bool enabled)
{
  pqPropertiesPanel::AutoApply = enabled;
}

//-----------------------------------------------------------------------------
bool pqPropertiesPanel::autoApply()
{
  return pqPropertiesPanel::AutoApply;
}

//-----------------------------------------------------------------------------
void pqPropertiesPanel::setAutoApplyDelay(int delay)
{
  pqPropertiesPanel::AutoApplyDelay = delay;
}

//-----------------------------------------------------------------------------
int pqPropertiesPanel::autoApplyDelay()
{
  return pqPropertiesPanel::AutoApplyDelay;
}

//-----------------------------------------------------------------------------
pqView* pqPropertiesPanel::view() const
{
  return this->Internals->View;
}

//-----------------------------------------------------------------------------
void pqPropertiesPanel::setRepresentation(pqDataRepresentation* repr)
{
  if (repr)
  {
    this->setView(repr->getView());
  }
  this->updateDisplayPanel(repr);
}

//-----------------------------------------------------------------------------
void pqPropertiesPanel::setView(pqView* pqview)
{
  this->updateViewPanel(pqview);
}

//-----------------------------------------------------------------------------
void pqPropertiesPanel::setOutputPort(pqOutputPort* port)
{
  this->updatePanel(port);
}

//-----------------------------------------------------------------------------
void pqPropertiesPanel::updatePanel()
{
  this->updatePanel(this->Internals->Port);
}

//-----------------------------------------------------------------------------
void pqPropertiesPanel::updatePanel(pqOutputPort* port)
{
  this->Internals->Port = port;

  // Determine if the proxy/repr has changed. If so, we have to recreate the
  // entire panel, else we simply update the widgets.
  this->updatePropertiesPanel(port ? port->getSource() : NULL);
  this->updateDisplayPanel(port ? port->getRepresentation(this->view()) : NULL);
  this->updateViewPanel(this->view());
  this->updateButtonState();
}

//-----------------------------------------------------------------------------
void pqPropertiesPanel::updatePropertiesPanel(pqPipelineSource* source)
{
  if ((this->PanelMode & SOURCE_PROPERTIES) == 0)
  {
    source = NULL;
  }

  if (this->Internals->Source != source)
  {
    // Panel has changed.
    if (this->Internals->Source && this->Internals->SourceWidgets.contains(this->Internals->Source))
    {
      this->Internals->SourceWidgets[this->Internals->Source]->hide();
    }
    this->Internals->Source = source;
    this->Internals->updateInformationAndDomains();

    if (source && !this->Internals->SourceWidgets.contains(source))
    {
      // create the panel for the source.
      pqProxyWidgets* widgets = new pqProxyWidgets(source, this);
      this->Internals->SourceWidgets[source] = widgets;

      QObject::connect(
        widgets->Panel, SIGNAL(changeAvailable()), this, SLOT(sourcePropertyChangeAvailable()));
      QObject::connect(
        widgets->Panel, SIGNAL(changeFinished()), this, SLOT(sourcePropertyChanged()));
    }

    if (source)
    {
      this->Internals->SourceWidgets[source]->show(this->Internals->Ui.PropertiesFrame);
    }
  }

  // update widgets.
  if (source)
  {
    this->Internals->Ui.PropertiesButton->setText(
      tr("Properties") + QString(" (%1)").arg(source->getSMName()));
    this->Internals->SourceWidgets[source]->showWidgets(
      this->Internals->Ui.SearchBox->isAdvancedSearchActive(),
      this->Internals->Ui.SearchBox->text());

    if (pqPropertiesPanel::AutoApply)
    {
      this->Internals->triggerAutoApply();
    }
  }
  else
  {
    this->Internals->Ui.PropertiesButton->setText(tr("Properties"));
  }
  this->updateButtonEnableState();
}

//-----------------------------------------------------------------------------
void pqPropertiesPanel::updateDisplayPanel()
{
  this->updateDisplayPanel(this->Internals->Representation);
}

//-----------------------------------------------------------------------------
void pqPropertiesPanel::updateDisplayPanel(pqDataRepresentation* repr)
{
  if ((this->PanelMode & pqPropertiesPanel::DISPLAY_PROPERTIES) == 0)
  {
    repr = NULL;
  }

  // since this->Internals->Representation is QPointer, it can go NULL (e.g. during
  // disconnect) before we get the chance to clear the panel's widgets. Hence we
  // do the block of code if (repr==NULL) event if nothing has changed.
  if (this->Internals->Representation != repr || repr == NULL)
  {
    // Representation has changed, destroy the current display panel and create
    // a new one. Unlike properties panels, display panels are not cached.
    if (this->Internals->DisplayWidgets)
    {
      this->Internals->DisplayWidgets->hide();
      delete this->Internals->DisplayWidgets;
    }
    this->Internals->RepresentationEventConnect->Disconnect();
    this->Internals->Representation = repr;
    if (repr)
    {
      if (repr->getProxy()->GetProperty("Representation"))
      {
        this->Internals->RepresentationEventConnect->Connect(
          repr->getProxy()->GetProperty("Representation"), vtkCommand::ModifiedEvent, this,
          SLOT(updateDisplayPanel()));
      }

      // create the panel for the repr.
      pqProxyWidgets* widgets = new pqProxyWidgets(repr, this);
      widgets->Panel->setApplyChangesImmediately(true);
      QObject::connect(widgets->Panel, SIGNAL(changeFinished()), this, SLOT(renderActiveView()));
      this->Internals->DisplayWidgets = widgets;
      this->Internals->DisplayWidgets->show(this->Internals->Ui.DisplayFrame);
    }
  }

  if (repr)
  {
    this->Internals->Ui.DisplayButton->setText(
      tr("Display") + QString(" (%1)").arg(repr->getProxy()->GetXMLName()));
    this->Internals->DisplayWidgets->showWidgets(
      this->Internals->Ui.SearchBox->isAdvancedSearchActive(),
      this->Internals->Ui.SearchBox->text());
  }
  else
  {
    this->Internals->Ui.DisplayButton->setText("Display");
  }

  this->updateButtonEnableState();
}

//-----------------------------------------------------------------------------
void pqPropertiesPanel::updateViewPanel(pqView* argView)
{
  pqView* _view = argView;
  if ((this->PanelMode & pqPropertiesPanel::VIEW_PROPERTIES) == 0)
  {
    _view = NULL;
  }

  if (this->Internals->View != argView)
  {
    // The view has changed.
    if (this->Internals->View)
    {
      if (this->Internals->ViewWidgets)
      {
        this->Internals->ViewWidgets->hide();
        delete this->Internals->ViewWidgets;
      }
    }
    this->Internals->View = argView;
    emit this->viewChanged(argView);
    if (_view)
    {
      // create the widgets for this view
      pqProxyWidgets* widgets = new pqProxyWidgets(_view, this);
      widgets->Panel->setApplyChangesImmediately(true);
      QObject::connect(widgets->Panel, SIGNAL(changeFinished()), this, SLOT(renderActiveView()));
      this->Internals->ViewWidgets = widgets;
      this->Internals->ViewWidgets->show(this->Internals->Ui.ViewFrame);
    }
  }

  if (_view)
  {
    // update the label and show the widgets
    vtkSMViewProxy* proxy = _view->getViewProxy();
    const char* label = proxy->GetXMLLabel();
    this->Internals->Ui.ViewButton->setText(
      tr("View") + QString(" (%1)").arg(label != 0 ? label : _view->getViewType()));
    this->Internals->ViewWidgets->showWidgets(
      this->Internals->Ui.SearchBox->isAdvancedSearchActive(),
      this->Internals->Ui.SearchBox->text());
  }
  else
  {
    this->Internals->Ui.ViewButton->setText(tr("View"));
  }

  this->updateButtonEnableState();
}

//-----------------------------------------------------------------------------
void pqPropertiesPanel::renderActiveView()
{
  if (this->view())
  {
    this->view()->render();
  }
}

//-----------------------------------------------------------------------------
void pqPropertiesPanel::sourcePropertyChanged(bool change_finished /*=true*/)
{
  std::string proxyLabel("(unknown)");
  if (auto signalSender = qobject_cast<pqProxyWidget*>(this->sender()))
  {
    if (auto proxy = signalSender->proxy())
    {
      proxyLabel = proxy->GetLogNameOrDefault();
    }
  }

  if (!change_finished)
  {
    this->Internals->ReceivedChangeAvailable = true;
  }
  if (change_finished && !this->Internals->ReceivedChangeAvailable)
  {
    vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "received `changeFinished` signal without "
                                                   "receiving a `changeAvailable` signal from "
                                                   "`%s`'s proxy-widget;"
                                                   "ignoring it!",
      proxyLabel.c_str());
    return;
  }

  vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "received `%s` from `%s`'s proxy-widget",
    (change_finished ? "changeFinished" : "changeAvailable"), proxyLabel.c_str());
  if (this->Internals->Source && this->Internals->Source->modifiedState() == pqProxy::UNMODIFIED)
  {
    this->Internals->Source->setModifiedState(pqProxy::MODIFIED);
  }
  if (pqPropertiesPanel::AutoApply && change_finished)
  {
    this->Internals->triggerAutoApply();
  }
  this->updateButtonState();
}

//-----------------------------------------------------------------------------
void pqPropertiesPanel::updateButtonState()
{
  const Ui::propertiesPanel& ui = this->Internals->Ui;

  const bool previous_apply_state = ui.Accept->isEnabled();

  ui.Accept->setEnabled(false);
  ui.Reset->setEnabled(false);
  ui.Help->setEnabled(this->Internals->Source != NULL);
  ui.Delete->setEnabled(this->Internals->Source != NULL &&
    this->Internals->Source->getNumberOfConsumers() == 0 &&
    !pqLiveInsituManager::isInsitu(this->Internals->Source));

  foreach (const pqProxyWidgets* widgets, this->Internals->SourceWidgets)
  {
    pqProxy* proxy = widgets->Proxy;
    if (proxy == NULL)
    {
      continue;
    }

    if (proxy->modifiedState() == pqProxy::UNINITIALIZED)
    {
      vtkVLogIfF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), previous_apply_state == false,
        "`Apply` button enabled since `%s` became uninitialized.",
        proxy->getProxy()->GetLogNameOrDefault());
      ui.Accept->setEnabled(true);
      ui.PropertiesRestoreDefaults->setEnabled(true);
      ui.PropertiesSaveAsDefaults->setEnabled(true);
    }
    else if (proxy->modifiedState() == pqProxy::MODIFIED)
    {
      vtkVLogIfF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), previous_apply_state == false,
        "`Apply` button enabled since `%s` became modified.",
        proxy->getProxy()->GetLogNameOrDefault());
      ui.Accept->setEnabled(true);
      ui.Reset->setEnabled(true);
      ui.PropertiesRestoreDefaults->setEnabled(true);
      ui.PropertiesSaveAsDefaults->setEnabled(true);
    }
  }

  vtkVLogIfF(PARAVIEW_LOG_APPLICATION_VERBOSITY(),
    (previous_apply_state && !ui.Accept->isEnabled()),
    "`Apply` button disabled since no changes are apply-able changes are present.");

  if (!ui.Accept->isEnabled())
  {
    // It's a good place to reset the ReceivedChangeAvailable if Accept button
    // is not enabled. This is same as doing it in apply()/reset() or if the
    // only modified proxy is deleted.
    this->Internals->ReceivedChangeAvailable = false;
  }

  emit this->applyEnableStateChanged();
  this->updateButtonEnableState();
}

//-----------------------------------------------------------------------------
/// Updates enabled state for buttons on panel (other than
/// apply/reset/delete);
void pqPropertiesPanel::updateButtonEnableState()
{
  pqInternals& internals = *this->Internals;
  Ui::propertiesPanel& ui = internals.Ui;

  // Update PropertiesSaveAsDefaults and PropertiesRestoreDefaults state.
  // If the source's properties are yet to be applied, we disable the two
  // buttons (see BUG #15338).
  bool canSaveRestoreSourcePropertyDefaults =
    internals.Source != NULL ? (internals.Source->modifiedState() == pqProxy::UNMODIFIED) : false;
  ui.PropertiesSaveAsDefaults->setEnabled(canSaveRestoreSourcePropertyDefaults);
  ui.PropertiesRestoreDefaults->setEnabled(canSaveRestoreSourcePropertyDefaults);

  ui.DisplayRestoreDefaults->setEnabled(internals.DisplayWidgets != NULL);
  ui.DisplaySaveAsDefaults->setEnabled(internals.DisplayWidgets != NULL);

  ui.ViewRestoreDefaults->setEnabled(internals.ViewWidgets != NULL);
  ui.ViewSaveAsDefaults->setEnabled(internals.ViewWidgets != NULL);

  // Now update copy-paste button state as well.
  if (internals.Source)
  {
    ui.PropertiesCopy->setEnabled(true);
    ui.PropertiesPaste->setEnabled(
      internals.SourceClipboard->CanPaste(internals.Source->getProxy()));
  }
  else
  {
    ui.PropertiesCopy->setEnabled(false);
    ui.PropertiesPaste->setEnabled(false);
  }
  if (internals.Representation)
  {
    ui.DisplayCopy->setEnabled(true);
    ui.DisplayPaste->setEnabled(
      internals.DisplayClipboard->CanPaste(internals.Representation->getProxy()));
  }
  else
  {
    ui.DisplayCopy->setEnabled(false);
    ui.DisplayPaste->setEnabled(false);
  }
  if (internals.View)
  {
    ui.ViewCopy->setEnabled(true);
    ui.ViewPaste->setEnabled(internals.ViewClipboard->CanPaste(internals.View->getProxy()));
  }
  else
  {
    ui.ViewCopy->setEnabled(false);
    ui.ViewPaste->setEnabled(false);
  }
}

//-----------------------------------------------------------------------------
void pqPropertiesPanel::apply()
{
  vtkVLogScopeFunction(PARAVIEW_LOG_APPLICATION_VERBOSITY());

  vtkTimerLog::MarkStartEvent("PropertiesPanel::Apply");
  this->Internals->AutoApplyTimer.stop();

  BEGIN_UNDO_SET("Apply");

  bool onlyApplyCurrentPanel = vtkPVGeneralSettings::GetInstance()->GetAutoApplyActiveOnly();

  // Grab focus. This ensures other widgets lose focus and take care of any state updating
  // they need to do when they lose focus. Workaround for macOS bug #18626.
  this->Internals->Ui.Accept->setFocus();

  if (onlyApplyCurrentPanel)
  {
    pqProxyWidgets* widgets =
      this->Internals->Source ? this->Internals->SourceWidgets[this->Internals->Source] : NULL;
    if (widgets)
    {
      widgets->apply(this->view());
      emit this->applied(widgets->Proxy);
    }
  }
  else
  {
    foreach (pqProxyWidgets* widgets, this->Internals->SourceWidgets)
    {
      widgets->apply(this->view());
      emit this->applied(widgets->Proxy);
    }
  }

  this->Internals->updateInformationAndDomains();
  this->updateButtonState();

  emit this->applied();
  END_UNDO_SET();
  vtkTimerLog::MarkEndEvent("PropertiesPanel::Apply");
}

//-----------------------------------------------------------------------------
void pqPropertiesPanel::reset()
{
  this->Internals->AutoApplyTimer.stop();

  bool onlyApplyCurrentPanel = vtkPVGeneralSettings::GetInstance()->GetAutoApplyActiveOnly();
  if (onlyApplyCurrentPanel)
  {
    pqProxyWidgets* widgets =
      this->Internals->Source ? this->Internals->SourceWidgets[this->Internals->Source] : NULL;
    if (widgets)
    {
      widgets->reset();
    }
  }
  else
  {
    foreach (pqProxyWidgets* widgets, this->Internals->SourceWidgets)
    {
      widgets->reset();
    }
  }

  this->Internals->updateInformationAndDomains();
  this->updateButtonState();
}

//-----------------------------------------------------------------------------
void pqPropertiesPanel::deleteProxy()
{
  if (this->Internals->Source)
  {
    BEGIN_UNDO_SET(tr("Delete") + " " + this->Internals->Source->getSMName());
    emit this->deleteRequested(this->Internals->Source);
    END_UNDO_SET();
  }
}

//-----------------------------------------------------------------------------
void pqPropertiesPanel::showHelp()
{
  if (this->Internals->Source)
  {
    this->helpRequested(this->Internals->Source->getProxy()->GetXMLGroup(),
      this->Internals->Source->getProxy()->GetXMLName());
  }
}

//-----------------------------------------------------------------------------
void pqPropertiesPanel::propertiesRestoreDefaults()
{
  pqProxyWidgets* widgets =
    this->Internals->Source ? this->Internals->SourceWidgets[this->Internals->Source] : NULL;
  if (widgets && widgets->Panel)
  {
    if (widgets->Panel->restoreDefaults())
    {
      // If defaults were restored, we're going to pretend that the user hit
      // apply for the source, so that the property changes are "accepted" and
      // rest of the application updates.
      widgets->apply(this->view());
      emit this->applied(widgets->Proxy);
      emit this->applied();
    }
  }
}

//-----------------------------------------------------------------------------
void pqPropertiesPanel::propertiesSaveAsDefaults()
{
  pqProxyWidgets* widgets =
    this->Internals->Source ? this->Internals->SourceWidgets[this->Internals->Source] : NULL;
  if (widgets && widgets->Panel)
  {
    widgets->Panel->saveAsDefaults();
  }
}

//-----------------------------------------------------------------------------
void pqPropertiesPanel::displayRestoreDefaults()
{
  if (this->Internals->DisplayWidgets)
  {
    if (this->Internals->DisplayWidgets->Panel->restoreDefaults())
    {
      this->Internals->DisplayWidgets->Panel->apply();
    }
  }
}

//-----------------------------------------------------------------------------
void pqPropertiesPanel::displaySaveAsDefaults()
{
  if (this->Internals->DisplayWidgets)
  {
    this->Internals->DisplayWidgets->Panel->saveAsDefaults();
  }
}

//-----------------------------------------------------------------------------
void pqPropertiesPanel::viewRestoreDefaults()
{
  if (this->Internals->ViewWidgets)
  {
    if (this->Internals->ViewWidgets->Panel->restoreDefaults())
    {
      this->Internals->ViewWidgets->Panel->apply();
    }
  }
}

//-----------------------------------------------------------------------------
void pqPropertiesPanel::viewSaveAsDefaults()
{
  if (this->Internals->ViewWidgets)
  {
    this->Internals->ViewWidgets->Panel->saveAsDefaults();
  }
}

//-----------------------------------------------------------------------------
void pqPropertiesPanel::proxyDeleted(pqPipelineSource* source)
{
  if (this->Internals->Source == source)
  {
    this->setOutputPort(NULL);
  }
  if (this->Internals->SourceWidgets.contains(source))
  {
    delete this->Internals->SourceWidgets[source];
    this->Internals->SourceWidgets.remove(source);
  }

  this->updateButtonState();
}

//-----------------------------------------------------------------------------
void pqPropertiesPanel::copyProperties()
{
  this->Internals->SourceClipboard->Copy(this->Internals->Source->getProxy());
  this->updateButtonEnableState();
}

//-----------------------------------------------------------------------------
void pqPropertiesPanel::pasteProperties()
{
  this->Internals->SourceClipboard->Paste(this->Internals->Source->getProxy());
  this->renderActiveView();
}

//-----------------------------------------------------------------------------
void pqPropertiesPanel::copyDisplay()
{
  this->Internals->DisplayClipboard->Copy(this->Internals->Representation->getProxy());
  this->updateButtonEnableState();
}

//-----------------------------------------------------------------------------
void pqPropertiesPanel::pasteDisplay()
{
  this->Internals->DisplayClipboard->Paste(this->Internals->Representation->getProxy());
  this->renderActiveView();
}

//-----------------------------------------------------------------------------
void pqPropertiesPanel::copyView()
{
  this->Internals->ViewClipboard->Copy(this->Internals->View->getProxy());
  this->updateButtonEnableState();
}

//-----------------------------------------------------------------------------
void pqPropertiesPanel::pasteView()
{
  this->Internals->ViewClipboard->Paste(this->Internals->View->getProxy());
  this->renderActiveView();
}
