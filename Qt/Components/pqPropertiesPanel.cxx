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
#include "pqDebug.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqProxyWidget.h"
#include "pqServerManagerModel.h"
#include "pqSettings.h"
#include "pqTimer.h"
#include "pqUndoStack.h"
#include "vtkCollection.h"
#include "vtkCommand.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkNew.h"
#include "vtkPVGeneralSettings.h"
#include "vtkSMProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMViewProxy.h"
#include "vtkTimerLog.h"

#include <QKeyEvent>
#include <QPointer>
#include <QStyleFactory>

#include <iostream>

namespace
{
  class pqClearTextOnEsc : public QObject
  {
public:
  pqClearTextOnEsc(QLineEdit* parentObject) : QObject(parentObject)
    {
    }
protected:
  virtual bool eventFilter(QObject *obj, QEvent *evt)
    {
    if (evt->type() == QEvent::KeyPress)
      {
      QKeyEvent *keyEvent = static_cast<QKeyEvent*>(evt);
      if (keyEvent->key() == Qt::Key_Escape)
        {
        qobject_cast<QLineEdit*>(this->parent())->clear();
        }
      }
    return this->QObject::eventFilter(obj, evt);
    }
  };

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
    pqProxyWidgets(pqProxy* proxy, pqPropertiesPanel* panel) :
      Superclass(panel)
      {
      this->MyId = pqProxyWidgets::Counter++;

      this->Proxy = proxy;
      this->Panel = new pqProxyWidget(proxy->getProxy());
      this->Panel->setObjectName(
        QString("HiddenProxyPanel%1").arg(this->MyId));
      this->Panel->setView(panel->view());
      QObject::connect(panel, SIGNAL(viewChanged(pqView*)),
                       this->Panel, SLOT(setView(pqView*)));
      }

    ~pqProxyWidgets()
      {
      delete this->Panel;
      }

    void hide()
      {
      this->Panel->setObjectName(
        QString("HiddenProxyPanel%1").arg(this->MyId));
      this->Panel->hide();
      this->Panel->parentWidget()->layout()->removeWidget(this->Panel);
      this->Panel->setParent(NULL);
      }

    void show(QWidget* parentWdg)
      {
      Q_ASSERT(parentWdg != NULL);

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

    void apply(pqView* view)
      {
      (void) view;
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
    Q_DISABLE_COPY(pqProxyWidgets);
    };

  unsigned long pqProxyWidgets::Counter = 0;
};

#define DEBUG_APPLY_BUTTON() pqDebug("PV_DEBUG_APPLY_BUTTON")

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

  vtkNew<vtkEventQtSlotConnect> RepresentationEventConnect;

  //---------------------------------------------------------------------------
  pqInternals(pqPropertiesPanel* panel): ReceivedChangeAvailable(false)
    {
    this->Ui.setupUi(panel);

    // Setup background color for Apply button so users notice it when it's
    // enabled.

    // if XP Style is being used swap it out for cleanlooks which looks
    // almost the same so we can have a green apply button make all
    // the buttons the same
    QString styleName = this->Ui.Accept->style()->metaObject()->className();
    if (styleName == "QWindowsXPStyle")
      {
      QStyle *styleLocal = QStyleFactory::create("cleanlooks");
      styleLocal->setParent(panel);
      this->Ui.Accept->setStyle(styleLocal);
      this->Ui.Reset->setStyle(styleLocal);
      this->Ui.Delete->setStyle(styleLocal);
      QPalette buttonPalette = this->Ui.Accept->palette();
      buttonPalette.setColor(QPalette::Button, QColor(244,246,244));
      this->Ui.Accept->setPalette(buttonPalette);
      this->Ui.Reset->setPalette(buttonPalette);
      this->Ui.Delete->setPalette(buttonPalette);
      }

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

  void triggerAutoApply()
    {
    this->AutoApplyTimer.start(pqPropertiesPanel::autoApplyDelay());
    }


  //---------------------------------------------------------------------------
  void updateInformationAndDomains()
    {
    if (this->Source)
      {
      // this->Source->updatePipeline();
      vtkSMProxy *proxy = this->Source->getProxy();
      if (vtkSMSourceProxy *sourceProxy = vtkSMSourceProxy::SafeDownCast(proxy))
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
  : Superclass(parentObject),
  Internals(new pqInternals(this)),
  PanelMode(pqPropertiesPanel::ALL_PROPERTIES)
{
  // Setup configuration defaults using settings.
  pqSettings *settings = pqApplicationCore::instance()->settings();
  if (settings)
    {
    this->Internals->Ui.AdvancedButton->setChecked(
      settings->value("showAdvancedProperties", false).toBool());
    }

  // Get AutoApply setting from GeneralSettings object.
  vtkPVGeneralSettings* generalSettings = vtkPVGeneralSettings::GetInstance();
  pqPropertiesPanel::AutoApply = generalSettings->GetAutoApply();

  // every time the settings change, we need to update our auto-apply status.
  pqCoreUtilities::connect(generalSettings, vtkCommand::ModifiedEvent,
    this, SLOT(generalSettingsChanged()));

  //---------------------------------------------------------------------------
  // Listen to various signals from the application indicating changes in active
  // source/view/representation, etc.
  pqActiveObjects *activeObjects = &pqActiveObjects::instance();
  this->connect(activeObjects, SIGNAL(portChanged(pqOutputPort*)),
                this, SLOT(setOutputPort(pqOutputPort*)));
  this->connect(activeObjects, SIGNAL(viewChanged(pqView*)),
                this, SLOT(setView(pqView*)));
  this->connect(activeObjects, SIGNAL(representationChanged(pqDataRepresentation*)),
                this, SLOT(setRepresentation(pqDataRepresentation*)));

  // listen to server manager changes
  pqServerManagerModel *smm = pqApplicationCore::instance()->getServerManagerModel();
  this->connect(smm, SIGNAL(sourceRemoved(pqPipelineSource*)),
    SLOT(proxyDeleted(pqPipelineSource*)));
  // this connection ensures that the button state is updated everytime any
  // item's state changes.
  this->connect(smm, SIGNAL(modifiedStateChanged(pqServerManagerModelItem*)),
    SLOT(updateButtonState()));
  this->connect(pqApplicationCore::instance()->getServerManagerModel(),
    SIGNAL(connectionRemoved(pqPipelineSource*, pqPipelineSource*, int)),
    SLOT(updateButtonState()));
  this->connect(pqApplicationCore::instance()->getServerManagerModel(),
    SIGNAL(connectionAdded(pqPipelineSource*, pqPipelineSource*, int)),
    SLOT(updateButtonState()));

  // Setup shortcut to clear search text.
  this->Internals->Ui.SearchLineEdit->installEventFilter(
    new pqClearTextOnEsc(this->Internals->Ui.SearchLineEdit));

  // Listen to UI signals.
  QObject::connect(this->Internals->Ui.Accept, SIGNAL(clicked()),
                   this, SLOT(apply()));
  QObject::connect(this->Internals->Ui.Reset, SIGNAL(clicked()),
                   this, SLOT(reset()));
  QObject::connect(this->Internals->Ui.Delete, SIGNAL(clicked()),
                   this, SLOT(deleteProxy()));
  QObject::connect(this->Internals->Ui.Help, SIGNAL(clicked()),
                   this, SLOT(showHelp()));
  QObject::connect(this->Internals->Ui.AdvancedButton, SIGNAL(toggled(bool)),
                   this, SLOT(updatePanel()));
  QObject::connect(this->Internals->Ui.SearchLineEdit, SIGNAL(textChanged(QString)),
                   this, SLOT(updatePanel()));

  QObject::connect(this->Internals->Ui.PropertiesButton, SIGNAL(toggled(bool)),
                   this->Internals->Ui.PropertiesFrame, SLOT(setVisible(bool)));
  QObject::connect(this->Internals->Ui.DisplayButton, SIGNAL(toggled(bool)),
                   this->Internals->Ui.DisplayFrame, SLOT(setVisible(bool)));
  QObject::connect(this->Internals->Ui.ViewButton, SIGNAL(toggled(bool)),
                   this->Internals->Ui.ViewFrame, SLOT(setVisible(bool)));


  this->setOutputPort(NULL);
}

//-----------------------------------------------------------------------------
pqPropertiesPanel::~pqPropertiesPanel()
{
  pqSettings *settings = pqApplicationCore::instance()->settings();
  if (settings)
    {
    // save the state of advanced button in the user config.
    settings->setValue("showAdvancedProperties",
      this->Internals->Ui.AdvancedButton->isChecked());
    }

  delete this->Internals;
  this->Internals = 0;
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

  this->Internals->Ui.PropertiesFrame->setVisible(has_source);
  this->Internals->Ui.ViewFrame->setVisible(has_view);
  this->Internals->Ui.DisplayFrame->setVisible(has_display);

  // the buttons need not be shown if there's only 1 type in the panel.
  bool has_multiples_types =
    ((has_source? 1 : 0) + (has_display? 1 : 0) + (has_view? 1 : 0)) > 1;
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
  this->updatePropertiesPanel(port? port->getSource() : NULL);
  this->updateDisplayPanel(port? port->getRepresentation(this->view()) : NULL);
  this->updateViewPanel(this->view());
  this->updateButtonState();
}

//-----------------------------------------------------------------------------
void pqPropertiesPanel::updatePropertiesPanel(pqPipelineSource *source)
{
  if ( (this->PanelMode & SOURCE_PROPERTIES) == 0)
    {
    source = NULL;
    }

  if (this->Internals->Source != source)
    {
    // Panel has changed.
    if (this->Internals->Source &&
      this->Internals->SourceWidgets.contains(this->Internals->Source))
      {
      this->Internals->SourceWidgets[
        this->Internals->Source]->hide();
      }
    this->Internals->Source = source;
    this->Internals->updateInformationAndDomains();

    if (source && !this->Internals->SourceWidgets.contains(source))
      {
      // create the panel for the source.
      pqProxyWidgets* widgets = new pqProxyWidgets(source, this);
      this->Internals->SourceWidgets[source] = widgets;

      QObject::connect(widgets->Panel, SIGNAL(changeAvailable()),
                       this, SLOT(sourcePropertyChangeAvailable()));
      QObject::connect(widgets->Panel, SIGNAL(changeFinished()),
                       this, SLOT(sourcePropertyChanged()));
      }

    if (source)
      {
      this->Internals->SourceWidgets[source]->show(
        this->Internals->Ui.PropertiesFrame);
      }
    }

  // update widgets.
  if (source)
    {
    this->Internals->Ui.PropertiesButton->setText(
      QString("Properties (%1)").arg(source->getSMName()));
    this->Internals->SourceWidgets[source]->showWidgets(
      this->Internals->Ui.AdvancedButton->isChecked(),
      this->Internals->Ui.SearchLineEdit->text());

    if (pqPropertiesPanel::AutoApply)
      {
      this->Internals->triggerAutoApply();
      }
    }
  else
    {
    this->Internals->Ui.PropertiesButton->setText("Properties");
    }
}

//-----------------------------------------------------------------------------
void pqPropertiesPanel::updateDisplayPanel()
{
  this->updateDisplayPanel(this->Internals->Representation);
}

//-----------------------------------------------------------------------------
void pqPropertiesPanel::updateDisplayPanel(pqDataRepresentation* repr)
{
  if ( (this->PanelMode & pqPropertiesPanel::DISPLAY_PROPERTIES) == 0)
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
          repr->getProxy()->GetProperty("Representation"),
          vtkCommand::ModifiedEvent, this, SLOT(updateDisplayPanel()));
        }

      // create the panel for the repr.
      pqProxyWidgets* widgets = new pqProxyWidgets(repr, this);
      widgets->Panel->setApplyChangesImmediately(true);
      QObject::connect(widgets->Panel, SIGNAL(changeFinished()),
                       this, SLOT(renderActiveView()));
      this->Internals->DisplayWidgets = widgets;
      this->Internals->DisplayWidgets->show(
        this->Internals->Ui.DisplayFrame);
      }
    }

  if (repr)
    {
    this->Internals->Ui.DisplayButton->setText(
      QString("Display (%1)").arg(repr->getProxy()->GetXMLName()));
    this->Internals->DisplayWidgets->showWidgets(
      this->Internals->Ui.AdvancedButton->isChecked(),
      this->Internals->Ui.SearchLineEdit->text());
    }
  else
    {
    this->Internals->Ui.DisplayButton->setText("Display");
    }
}

//-----------------------------------------------------------------------------
void pqPropertiesPanel::updateViewPanel (pqView* _view)
{
  if ( (this->PanelMode & pqPropertiesPanel::VIEW_PROPERTIES) == 0)
    {
    _view = NULL;
    }

  if (this->Internals->View != _view)
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
    this->Internals->View = _view;
    emit this->viewChanged(_view);
    if (_view)
      {
      // create the widgets for this view
      pqProxyWidgets* widgets = new pqProxyWidgets(_view, this);
      widgets->Panel->setApplyChangesImmediately(true);
      QObject::connect(widgets->Panel, SIGNAL(changeFinished()),
                       this, SLOT(renderActiveView()));
      this->Internals->ViewWidgets = widgets;
      this->Internals->ViewWidgets->show(this->Internals->Ui.ViewFrame);
      }
    }

  if (_view)
    {
    // update the label and show the widgets
    vtkSMViewProxy* proxy = _view->getViewProxy();
    const char* label = proxy->GetXMLLabel ();
    this->Internals->Ui.ViewButton->setText(
      QString ("View (%1)").arg (label != 0 ? label : _view->getViewType ()));
    this->Internals->ViewWidgets->showWidgets(
      this->Internals->Ui.AdvancedButton->isChecked(),
      this->Internals->Ui.SearchLineEdit->text());
    }
  else
    {
    this->Internals->Ui.ViewButton->setText("View");
    }
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
void pqPropertiesPanel::sourcePropertyChanged(bool change_finished/*=true*/)
{
  // FIXME:
  QString senderClass("(unknown)");
  QString senderLabel("(unknown)");
  QObject *signalSender = this->sender();
  if (signalSender)
    {
    senderClass = signalSender->metaObject()->className();
    //pqPropertyWidget *senderWidget = qobject_cast<pqPropertyWidget *>(signalSender);
    //if (senderWidget)
    //  {
    //  senderLabel = senderWidget->property()->GetXMLLabel();
    //  }
    }

  if (!change_finished)
    {
    this->Internals->ReceivedChangeAvailable = true;
    }
  if (change_finished && !this->Internals->ReceivedChangeAvailable)
    {
    DEBUG_APPLY_BUTTON()
      << "Received change-finished before change-available. Ignoring it.";
    return;
    }

  DEBUG_APPLY_BUTTON()
      << "Property change "
      << (change_finished? "finished" : "available")
      << senderLabel << "(" << senderClass << ")";

  if (this->Internals->Source &&
      this->Internals->Source->modifiedState() == pqProxy::UNMODIFIED)
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
  this->Internals->Ui.Accept->setEnabled(false);
  this->Internals->Ui.Reset->setEnabled(false);

  // disable advanced button when searching.
  this->Internals->Ui.AdvancedButton->setEnabled(
    this->Internals->Ui.SearchLineEdit->text().isEmpty());

  this->Internals->Ui.Help->setEnabled(
    this->Internals->Source != NULL);

  this->Internals->Ui.Delete->setEnabled(
    (this->Internals->Source != NULL &&
     this->Internals->Source->getNumberOfConsumers() == 0));

  foreach (const pqProxyWidgets* widgets, this->Internals->SourceWidgets)
    {
    pqProxy* proxy = widgets->Proxy;
    if (proxy == NULL)
      {
      continue;
      }

    if (proxy->modifiedState() == pqProxy::UNINITIALIZED)
      {
      DEBUG_APPLY_BUTTON()
        << "Enabling the Apply button because the "
        << (proxy->getProxy() ? proxy->getProxy()->GetXMLName() : "(unknown)")
        << "proxy is uninitialized";

      this->Internals->Ui.Accept->setEnabled(true);
      }
    else if (proxy->modifiedState() == pqProxy::MODIFIED)
      {
      DEBUG_APPLY_BUTTON()
       << "Enabling the Apply button because the "
       << (proxy->getProxy() ? proxy->getProxy()->GetXMLName() : "(unknown)")
       << "proxy is modified";

      this->Internals->Ui.Accept->setEnabled(true);
      this->Internals->Ui.Reset->setEnabled(true);
      }
    }

  if (!this->Internals->Ui.Accept->isEnabled())
    {
    // It's a good place to reset the ReceivedChangeAvailable if Accept button
    // is not enabled. This is same as doing it in apply()/reset() or if the
    // only modified proxy is deleted.
    this->Internals->ReceivedChangeAvailable = false;
    }
}

//-----------------------------------------------------------------------------
void pqPropertiesPanel::apply()
{
  vtkTimerLog::MarkStartEvent("PropertiesPanel::Apply");
  this->Internals->AutoApplyTimer.stop();

  BEGIN_UNDO_SET("Apply");

  bool onlyApplyCurrentPanel =
    vtkPVGeneralSettings::GetInstance()->GetAutoApplyActiveOnly();

  if (onlyApplyCurrentPanel)
    {
    pqProxyWidgets* widgets = this->Internals->Source?
      this->Internals->SourceWidgets[this->Internals->Source] : NULL;
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

  bool onlyApplyCurrentPanel =
    vtkPVGeneralSettings::GetInstance()->GetAutoApplyActiveOnly();
  if (onlyApplyCurrentPanel)
    {
    pqProxyWidgets* widgets = this->Internals->Source?
      this->Internals->SourceWidgets[this->Internals->Source] : NULL;
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
    BEGIN_UNDO_SET(QString("Delete %1").arg(
        this->Internals->Source->getSMName()));
    emit this->deleteRequested(this->Internals->Source);
    END_UNDO_SET();
    }
}

//-----------------------------------------------------------------------------
void pqPropertiesPanel::showHelp()
{
  if (this->Internals->Source)
    {
    this->helpRequested(
      this->Internals->Source->getProxy()->GetXMLGroup(),
      this->Internals->Source->getProxy()->GetXMLName());
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
