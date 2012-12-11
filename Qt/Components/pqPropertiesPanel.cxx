/*=========================================================================

   Program: ParaView
   Module: pqPropertiesPanel.cxx

   Copyright (c) 2005-2012 Sandia Corporation, Kitware Inc.
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

#include "pqPropertiesPanel.h"
#include "ui_pqPropertiesPanel.h"

#include <QDebug>
#include <QLabel>
#include <QStyleFactory>

#include "vtkSMProxy.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMProxyListDomain.h"
#include "vtkSMOrderedPropertyIterator.h"
#include "vtkPVXMLElement.h"
#include "vtkSMDomain.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMDomainIterator.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkEventQtSlotConnect.h"
#include <vtksys/SystemTools.hxx>

#include "pqView.h"
#include "pqProxy.h"
#include "pq3DWidget.h"
#include "pqSettings.h"
#include "pqUndoStack.h"
#include "pqOutputPort.h"
#include "pqDisplayPanel.h"
#include "pqActiveObjects.h"
#include "pqDisplayPolicy.h"
#include "pqObjectBuilder.h"
#include "pqPipelineFilter.h"
#include "pqApplicationCore.h"
#include "pqInterfaceTracker.h"
#include "pqServerManagerModel.h"
#include "pqPropertiesPanelItem.h"
#include "pqProxyPropertiesPanel.h"
#include "pqObjectPanelInterface.h"
#include "pqProxySelectionWidget.h"
#include "pq3DWidgetPropertyWidget.h"
#include "pqPropertyWidgetInterface.h"
#include "pqObjectPanelPropertyWidget.h"
#include "pqDisplayPanelPropertyWidget.h"
#include "pqProxyModifiedStateUndoElement.h"

// custom panels
#include "pqClipPanel.h"
#include "pqGlyphPanel.h"
#include "pqNetCDFPanel.h"
#include "pqContourPanel.h"
#include "pqExodusIIPanel.h"
#include "pqThresholdPanel.h"
#include "pqIsoVolumePanel.h"
#include "pqPassArraysPanel.h"
#include "pqTextRepresentation.h"
#include "pqProxyPropertyWidget.h"
#include "pqXYChartDisplayPanel.h"
#include "pqExtractCTHPartsPanel.h"
#include "pqDisplayPanelInterface.h"
#include "pqPlotMatrixDisplayPanel.h"
#include "pqSpreadSheetDisplayEditor.h"
#include "pqTextDisplayPropertiesWidget.h"
#include "pqYoungsMaterialInterfacePanel.h"
#include "pqParallelCoordinatesChartDisplayPanel.h"

#include "pqPropertyWidget.h"
#include "pqIntVectorPropertyWidget.h"
#include "pqDoubleVectorPropertyWidget.h"
#include "pqStringVectorPropertyWidget.h"

bool pqPropertiesPanel::AutoApply = false;
int pqPropertiesPanel::AutoApplyDelay = 0; // in msec

// === pqStandardCustomPanels ============================================== //

namespace {

class pqStandardCustomPanels : public QObject, public pqObjectPanelInterface
{
public:
  pqStandardCustomPanels(QObject* p = 0)
    : QObject(p)
  {
  }

  pqObjectPanel* createPanel(pqProxy* proxy, QWidget* p)
  {
    if(QString("filters") == proxy->getProxy()->GetXMLGroup())
      {
      if(QString("Clip") == proxy->getProxy()->GetXMLName() ||
         QString("GenericClip") == proxy->getProxy()->GetXMLName())
        {
        return new pqClipPanel(proxy, p);
        }
      if(QString("PassArrays") == proxy->getProxy()->GetXMLName())
        {
        return new pqPassArraysPanel(proxy, p);
        }
      if (QString("ArbitrarySourceGlyph") == proxy->getProxy()->GetXMLName() ||
        QString("Glyph") == proxy->getProxy()->GetXMLName())
        {
        return new pqGlyphPanel(proxy, p);
        }
//      if(QString("ParticleTracer") == proxy->getProxy()->GetXMLName())
//        {
//        return new pqParticleTracerPanel(proxy, p);
//        }
      if(QString("Threshold") == proxy->getProxy()->GetXMLName())
        {
        return new pqThresholdPanel(proxy, p);
        }
      if(QString("IsoVolume") == proxy->getProxy()->GetXMLName())
        {
        return new pqIsoVolumePanel(proxy, p);
        }
      if(QString("Contour") == proxy->getProxy()->GetXMLName() ||
         QString("GenericContour") == proxy->getProxy()->GetXMLName())
        {
        return new pqContourPanel(proxy, p);
        }
      if(QString("CTHPart") == proxy->getProxy()->GetXMLName())
        {
        return new pqExtractCTHPartsPanel(proxy, p);
        }
      if(QString("RectilinearGridConnectivity") == proxy->getProxy()->GetXMLName())
        {
        // allow RectilinearGridConnectivity to reuse the panel of CTHPart
        return new pqExtractCTHPartsPanel(proxy, p);
        }
      if (QString("YoungsMaterialInterface") == proxy->getProxy()->GetXMLName())
        {
        return new pqYoungsMaterialInterfacePanel(proxy, p);
        }
      }
    return 0;
  }

  bool canCreatePanel(pqProxy* proxy) const
  {
    if(QString("filters") == proxy->getProxy()->GetXMLGroup())
      {
      if(QString("Clip") == proxy->getProxy()->GetXMLName() ||
         QString("GenericClip") == proxy->getProxy()->GetXMLName() ||
         QString("ArbitrarySourceGlyph") == proxy->getProxy()->GetXMLName() ||
         QString("Glyph") == proxy->getProxy()->GetXMLName() ||
//         QString("ExtractDataSets") == proxy->getProxy()->GetXMLName() ||
//         QString("ParticleTracer") == proxy->getProxy()->GetXMLName() ||
         QString("Threshold") == proxy->getProxy()->GetXMLName() ||
         QString("IsoVolume") == proxy->getProxy()->GetXMLName() ||
         QString("ExtractSelection") == proxy->getProxy()->GetXMLName() ||
         QString("ExtractSelectionOverTime") == proxy->getProxy()->GetXMLName() ||
         QString("Contour") == proxy->getProxy()->GetXMLName() ||
         QString("GenericContour") == proxy->getProxy()->GetXMLName() ||
         QString("CTHPart") == proxy->getProxy()->GetXMLName() ||
         QString("RectilinearGridConnectivity") == proxy->getProxy()->GetXMLName() ||
         QString("YoungsMaterialInterface") == proxy->getProxy()->GetXMLName() ||
         QString("PassArrays") == proxy->getProxy()->GetXMLName() )
        {
        return true;
        }
      }
    return false;
  }
};
}

// === pqStandardDisplayPanels ============================================= //
class pqStandardDisplayPanels : public QObject, public pqDisplayPanelInterface
{
public:
  /// constructor
  pqStandardDisplayPanels(){}
  /// destructor
  virtual ~pqStandardDisplayPanels(){}

  /// Returns true if this panel can be created for the given the proxy.
  virtual bool canCreatePanel(pqRepresentation* proxy) const
    {
    if(!proxy || !proxy->getProxy())
      {
      return false;
      }

    QString type = proxy->getProxy()->GetXMLName();

    if (type == "XYPlotRepresentation" ||
       type == "XYChartRepresentation" ||
       type == "XYBarChartRepresentation" ||
       type == "BarChartRepresentation" ||
       type == "SpreadSheetRepresentation" ||
       qobject_cast<pqTextRepresentation*>(proxy)||
       type == "ScatterPlotRepresentation" ||
       type == "ParallelCoordinatesRepresentation" ||
       type == "PlotMatrixRepresentation")
      {
      return true;
      }

    return false;
    }
  /// Creates a panel for the given proxy
  virtual pqDisplayPanel* createPanel(pqRepresentation* proxy, QWidget* p)
    {
    if(!proxy || !proxy->getProxy())
      {
      qDebug() << "Proxy is null" << proxy;
      return NULL;
      }

    QString type = proxy->getProxy()->GetXMLName();
    if (type == QString("XYChartRepresentation"))
      {
      return new pqXYChartDisplayPanel(proxy, p);
      }
    if (type == QString("XYBarChartRepresentation"))
      {
      return new pqXYChartDisplayPanel(proxy, p);
      }
    if (type == "SpreadSheetRepresentation")
      {
      return new pqSpreadSheetDisplayEditor(proxy, p);
      }

    if (qobject_cast<pqTextRepresentation*>(proxy))
      {
      return new pqTextDisplayPropertiesWidget(proxy, p);
      }
#ifdef FIXME
    if (type == "ScatterPlotRepresentation")
      {
      return new pqScatterPlotDisplayPanel(proxy, p);
      }
#endif
    if (type == QString("ParallelCoordinatesRepresentation"))
      {
      return new pqParallelCoordinatesChartDisplayPanel(proxy, p);
      }
    else if (type == "PlotMatrixRepresentation")
      {
      return new pqPlotMatrixDisplayPanel(proxy, p);
      }

    return NULL;
    }
};

pqPropertiesPanel::pqPropertiesPanel(QWidget *p)
  : QWidget(p),
    Ui(new Ui::pqPropertiesPanel)
{
  // enable debugging of the panel widgets if the PV_DEBUG_PANELS
  // enviornmental variable is set
  this->DebugWidgetCreation = 
    vtksys::SystemTools::GetEnv("PV_DEBUG_PANELS") != NULL;

  // enable debugging of the apply button state if the PV_DEBUG_APPLY_BUTTON
  // enviornmental variable is set
  this->DebugApplyButtonState =
    vtksys::SystemTools::GetEnv("PV_DEBUG_APPLY_BUTTON") != NULL;

  // enable auto apply for the panel if it enabled in the
  // global paraview settings
  pqSettings *settings = pqApplicationCore::instance()->settings();
  if(settings)
    {
    pqPropertiesPanel::AutoApply = settings->value("autoAccept", false).toBool();
    }
  else
    {
    pqPropertiesPanel::AutoApply = false;
    }

  this->Ui->setupUi(this);

  this->setObjectName("propertiesPanel");

  // setup buttons (apply, reset, delete, help)
  this->Ui->ApplyButton->setObjectName("Accept");
  this->Ui->ApplyButton->setIcon(QIcon(":/pqWidgets/Icons/pqUpdate16.png"));
  connect(this->Ui->ApplyButton, SIGNAL(clicked()), this, SLOT(apply()));
  this->Ui->ApplyButton->setDefault(true);
  this->Ui->ApplyButton->setEnabled(false);

  this->Ui->ResetButton->setObjectName("Reset");
  this->Ui->ResetButton->setIcon(QIcon(":/pqWidgets/Icons/pqCancel16.png"));
  connect(this->Ui->ResetButton, SIGNAL(clicked()), this, SLOT(reset()));
  this->Ui->ResetButton->setEnabled(false);

  this->Ui->DeleteButton->setObjectName("Delete");
  this->Ui->DeleteButton->setIcon(QIcon(":/QtWidgets/Icons/pqDelete16.png"));
  connect(this->Ui->DeleteButton, SIGNAL(clicked()), this, SLOT(deleteProxy()));
  this->Ui->DeleteButton->setEnabled(false);

  this->Ui->HelpButton->setObjectName("Help");
  this->Ui->HelpButton->setText("");
  this->Ui->HelpButton->setIcon(QIcon(":/pqWidgets/Icons/pqHelp16.png"));
  connect(this->Ui->HelpButton, SIGNAL(clicked()), this, SLOT(showHelp()));
  this->Ui->HelpButton->setEnabled(false);

  // if XP Style is being used swap it out for cleanlooks which looks
  // almost the same so we can have a green apply button make all
  // the buttons the same
  QString styleName = this->Ui->ApplyButton->style()->metaObject()->className();
  if(styleName == "QWindowsXPStyle")
     {
     QStyle *styleLocal = QStyleFactory::create("cleanlooks");
     styleLocal->setParent(this);
     this->Ui->ApplyButton->setStyle(styleLocal);
     this->Ui->ResetButton->setStyle(styleLocal);
     this->Ui->DeleteButton->setStyle(styleLocal);
     QPalette buttonPalette = this->Ui->ApplyButton->palette();
     buttonPalette.setColor(QPalette::Button, QColor(244,246,244));
     this->Ui->ApplyButton->setPalette(buttonPalette);
     this->Ui->ResetButton->setPalette(buttonPalette);
     this->Ui->DeleteButton->setPalette(buttonPalette);
     }

  // change the apply button palette so it is green when it is active
  QPalette applyPalette = this->Ui->ApplyButton->palette();
  applyPalette.setColor(QPalette::Active, QPalette::Button, QColor(161, 213, 135));
  this->Ui->ApplyButton->setPalette(applyPalette);

  // listen to active object changes
  pqActiveObjects *activeObjects = &pqActiveObjects::instance();
  this->connect(activeObjects, SIGNAL(portChanged(pqOutputPort*)),
                this, SLOT(setOutputPort(pqOutputPort*)));
  this->connect(activeObjects, SIGNAL(viewChanged(pqView*)),
                this, SLOT(setView(pqView*)));
  this->connect(activeObjects, SIGNAL(representationChanged(pqRepresentation*)),
                this, SLOT(setRepresentation(pqRepresentation*)));

  // listen to server manager changes
  pqServerManagerModel *smm = pqApplicationCore::instance()->getServerManagerModel();
  this->connect(smm, SIGNAL(sourceRemoved(pqPipelineSource*)),
                this, SLOT(removeProxy(pqPipelineSource*)));
  this->connect(smm, SIGNAL(connectionRemoved(pqPipelineSource*, pqPipelineSource*, int)),
                this, SLOT(handleConnectionChanged(pqPipelineSource*, pqPipelineSource*)));
  this->connect(smm, SIGNAL(connectionAdded(pqPipelineSource*, pqPipelineSource*, int)),
                this, SLOT(handleConnectionChanged(pqPipelineSource*, pqPipelineSource*)));

  // connect the apply button to the apply slot
  this->connect(this->Ui->ApplyButton, SIGNAL(clicked()),
                this, SLOT(apply()));

  // connect search string
  this->connect(this->Ui->SearchLineEdit, SIGNAL(textChanged(QString)),
                this, SLOT(searchTextChanged(QString)));

  this->esc = new QShortcut(Qt::Key_Escape,this);
  QObject::connect(this->esc,SIGNAL(activated()),this,SLOT(clearSearchText()));

  // connect to advanced button
  this->connect(this->Ui->AdvancedButton, SIGNAL(toggled(bool)),
                this, SLOT(advancedButtonToggled(bool)));

  this->connect(this->Ui->PropertiesButton, SIGNAL(toggled(bool)),
                this->Ui->PropertiesFrame, SLOT(setVisible(bool)));

  this->connect(this->Ui->DisplayButton, SIGNAL(toggled(bool)),
                this->Ui->DisplayFrame, SLOT(setVisible(bool)));
}

pqPropertiesPanel::~pqPropertiesPanel()
{
  delete this->esc;
  delete this->Ui;
}

void pqPropertiesPanel::setView(pqView *activeView)
{
  this->View = activeView;

  emit this->viewChanged(activeView);
}

pqView* pqPropertiesPanel::view() const
{
  return this->View;
}

pqPropertyWidget* pqPropertiesPanel::createWidgetForProperty(vtkSMProperty *property,
                                                             vtkSMProxy *proxy,
                                                             QWidget *parent)
{
  // check for custom widgets
  pqInterfaceTracker *interfaceTracker =
    pqApplicationCore::instance()->interfaceTracker();
  foreach(pqPropertyWidgetInterface *interface,
          interfaceTracker->interfaces<pqPropertyWidgetInterface *>())
    {
    pqPropertyWidget *widget =
      interface->createWidgetForProperty(proxy, property);

    if(widget)
      {
      // stop if we successfully created a property widget
      return widget;
      }
    }

  if(vtkSMDoubleVectorProperty *dvp = vtkSMDoubleVectorProperty::SafeDownCast(property))
    {
    return new pqDoubleVectorPropertyWidget(dvp, proxy, parent);
    }
  else if(vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(property))
    {
    return new pqIntVectorPropertyWidget(ivp, proxy, parent);
    }
  else if(vtkSMStringVectorProperty *svp = vtkSMStringVectorProperty::SafeDownCast(property))
    {
    return new pqStringVectorPropertyWidget(svp, proxy, parent);
    }
  else if(vtkSMProxyProperty *pp = vtkSMProxyProperty::SafeDownCast(property))
    {
    bool selection_input = (pp->GetHints() &&
      pp->GetHints()->FindNestedElementByName("SelectionInput"));

    // find the domain
    vtkSMDomain *domain = 0;
    vtkSMDomainIterator *domainIter = pp->NewDomainIterator();
    for(domainIter->Begin(); !domainIter->IsAtEnd(); domainIter->Next())
      {
      domain = domainIter->GetDomain();
      }
    domainIter->Delete();

    if (selection_input || vtkSMProxyListDomain::SafeDownCast(domain))
      {
      return new pqProxyPropertyWidget(pp, proxy, parent);
      }
    }

  return 0;
}

void pqPropertiesPanel::setProxy(pqProxy *proxy)
{
  // set current proxy
  this->Proxy = proxy;
  this->updateInformationAndDomains();

  // clear any search string
  this->Ui->SearchLineEdit->clear();

  // update group box name
  if(proxy)
    {
    this->Ui->PropertiesButton->setText(QString("Properties (%1)").arg(proxy->getSMName()));
    }
  else
    {
    this->Ui->PropertiesButton->setText("Properties");
    }

  // hide all proxy panel widgets
  foreach(pqProxyPropertiesPanel *panel, this->ProxyPanels.values())
    {
    if(panel)
      {
      panel->setObjectName("HiddenProxyPanel");
      panel->hide();
      }
    }

  if(!proxy)
    {
    this->Ui->ApplyButton->setEnabled(false);
    this->Ui->ResetButton->setEnabled(false);
    this->Ui->DeleteButton->setEnabled(false);
    this->Ui->HelpButton->setEnabled(false);
    return;
    }

  pqProxyPropertiesPanel *panel = this->ProxyPanels.value(proxy, 0);
  if(!panel)
    {
    // create a new panel for the proxy
    if(this->DebugWidgetCreation)
      {
      qDebug() << "Creating panel widgets for the"
               << proxy->getProxy()->GetXMLLabel() << "proxy:";
      }

    // search for a custom panel for the proxy
    pqObjectPanel *customPanel = 0;
    pqInterfaceTracker *interfaceTracker =
        pqApplicationCore::instance()->interfaceTracker();
    QObjectList interfaces = interfaceTracker->interfaces();
    foreach(QObject *interface, interfaces)
      {
      pqObjectPanelInterface *piface = qobject_cast<pqObjectPanelInterface*>(interface);
      if(piface && piface->canCreatePanel(proxy))
        {
        customPanel = piface->createPanel(proxy, 0);
        break;
        }
      }

    // try using the standard custom panels
    if(!customPanel)
      {
      pqStandardCustomPanels standardCustomPanels;

      if(standardCustomPanels.canCreatePanel(proxy))
        {
        customPanel = standardCustomPanels.createPanel(proxy, 0);
        }
      }

    // create property widgets
    QList<pqPropertiesPanelItem> widgets;

    if(customPanel)
      {
      // set view for panel
      customPanel->setView(this->View);
      connect(this, SIGNAL(viewChanged(pqView*)),
              customPanel, SLOT(setView(pqView*)));

      // must call select
      customPanel->select();

      pqPropertiesPanelItem item;
      item.Name = proxy->getProxy()->GetXMLName();
      item.LabelWidget = 0;
      item.PropertyWidget = new pqObjectPanelPropertyWidget(customPanel);
      item.IsAdvanced = false;

      if(this->DebugWidgetCreation)
        {
        qDebug() << "  - Using custom object panel:"
                 << customPanel->metaObject()->className();
        }
      widgets.append(item);
      }
    else
      {
      widgets = this->createWidgetsForProxy(proxy);
      }

    // add 3D widgets
    vtkSMProxy *smProxy = proxy->getProxy();
    vtkPVXMLElement *hints = smProxy->GetHints();
    if(hints)
      {
      QList<pq3DWidget*> widgets3d = pq3DWidget::createWidgets(smProxy, smProxy);

      foreach (pq3DWidget *widget, widgets3d)
        {
        connect(this, SIGNAL(viewChanged(pqView*)),
                widget, SLOT(setView(pqView*)));
        widget->setView(this->view());
        widget->resetBounds();
        widget->reset();

        // must call select
        widget->select();

        pqPropertiesPanelItem item;
        item.LabelWidget = 0;
        item.PropertyWidget = new pq3DWidgetPropertyWidget(widget);
        item.IsAdvanced = false;
        widgets.append(item);
        }
      }

    // create new proxy panel
    panel = new pqProxyPropertiesPanel(proxy, this);

    // add widgets to the panel
    foreach(const pqPropertiesPanelItem &item, widgets)
      {
      panel->addPropertyWidgetItem(item);

      // connect to modified signal
      this->connect(item.PropertyWidget, SIGNAL(modified()),
                    this, SLOT(proxyPropertyChanged()));
      }

    // set layout
    this->Ui->PropertiesLayout->addRow(panel);

    // store proxy panel
    this->ProxyPanels[proxy] = panel;
    }

  // set button state
  this->Ui->DeleteButton->setEnabled(true);
  this->Ui->HelpButton->setEnabled(true);

  // show the current proxy panel widget
  panel->setObjectName("ProxyPanel");
  panel->show();

  // update advanced state
  this->advancedButtonToggled(this->Ui->AdvancedButton->isChecked());

  // update apply button state
  this->updateButtonState();

  // auto-apply if enabled
  if(pqPropertiesPanel::AutoApply)
    {
    QTimer::singleShot(pqPropertiesPanel::AutoApplyDelay, this, SLOT(apply()));
    }
}

void pqPropertiesPanel::setOutputPort(pqOutputPort *port)
{
  if(port == this->OutputPort)
    {
    // nothing to change
    return;
    }

  this->OutputPort = port;

  if(port)
    {
    this->setProxy(port->getSource());
    }
  else
    {
    this->setProxy(0);
    }
}

void pqPropertiesPanel::setRepresentation(pqRepresentation *repr)
{
  if(repr == this->Representation)
    {
    // nothing to change
    return;
    }

  this->Representation = repr;

  // remove old property widgets
  foreach(const pqPropertiesPanelItem &item, this->RepresentationPropertyItems)
    {
    if(item.LabelWidget)
      {
      this->Ui->DisplayLayout->removeWidget(item.LabelWidget);
      delete item.LabelWidget;
      }

    this->Ui->DisplayLayout->removeWidget(item.PropertyWidget);
    delete item.PropertyWidget;
    }

  this->RepresentationPropertyItems.clear();

  // update group box name
  if(repr)
    {
    this->Ui->DisplayButton->setText(QString("Display (%1)").arg(repr->getProxy()->GetXMLName()));
    }
  else
    {
    this->Ui->DisplayButton->setText("Display");
    }

  if(!repr)
    {
    return;
    }

  if(this->DebugWidgetCreation)
    {
    qDebug() << "Creating panel widgets for the"
             << repr->getProxy()->GetXMLLabel() << "representation:";
    }

  QList<pqPropertiesPanelItem> widgets;

  pqDisplayPanel *customPanel = 0;
  pqStandardDisplayPanels standardDisplayPanels;
  if(standardDisplayPanels.canCreatePanel(repr))
    {
    customPanel = standardDisplayPanels.createPanel(repr, 0);
    }

  if(customPanel)
    {
    customPanel->dataUpdated();

    pqPropertiesPanelItem item;
    item.Name = repr->getProxy()->GetXMLName();
    item.LabelWidget = 0;
    item.PropertyWidget = new pqDisplayPanelPropertyWidget(customPanel);
    item.IsAdvanced = false;
    widgets.append(item);

    if(this->DebugWidgetCreation)
      {
      qDebug() << "  - Using custom display panel:"
               << customPanel->metaObject()->className();
      }
    }
  else
    {
    // create property widgets
    widgets = this->createWidgetsForProxy(repr);
    }

  foreach(const pqPropertiesPanelItem &item, widgets)
    {
    this->RepresentationPropertyItems.append(item);

    if(item.LabelWidget)
      {
      this->Ui->DisplayLayout->addRow(item.LabelWidget, item.PropertyWidget);
      }
    else
      {
      this->Ui->DisplayLayout->addRow(item.PropertyWidget);
      }

    if(item.PropertyWidget)
      {
      // automatically update vtk objects and re-render view on change
      item.PropertyWidget->setAutoUpdateVTKObjects(true);
      item.PropertyWidget->setUseUncheckedProperties(false);

      if(this->View)
        {
        this->connect(item.PropertyWidget, SIGNAL(modified()),
                      this->View, SLOT(render()));
        this->connect(item.PropertyWidget, SIGNAL(modified()),
                      item.PropertyWidget, SLOT(updateDependentDomains()));
        }
      }
    }

  // connect to representation type changed signal
  this->RepresentationTypeSignal->Disconnect();

  if(repr)
    {
    this->RepresentationTypeSignal->Connect(repr->getProxy(),
                                            vtkCommand::PropertyModifiedEvent,
                                            this,
                                            SLOT(representationPropertyChanged(vtkObject*,
                                                                               unsigned long,
                                                                               void*)));
    }

  // update advanced state
  this->advancedButtonToggled(this->Ui->AdvancedButton->isChecked());
}

void pqPropertiesPanel::apply()
{
  BEGIN_UNDO_SET("Apply");

  foreach(pqProxy *proxy, this->ProxyPanels.keys())
    {
    if(!proxy)
      {
      this->ProxyPanels.remove(proxy);
      continue;
      }

    // apply each property widget
    pqProxyPropertiesPanel *panel = this->ProxyPanels[proxy];
    if(panel)
      {
      panel->apply();
      }

    pqPipelineSource *source = qobject_cast<pqPipelineSource*>(proxy);
    if(source)
      {
      if(proxy->modifiedState() == pqProxy::UNINITIALIZED)
        {
        this->show(source);

        pqProxyModifiedStateUndoElement* undoElement =
            pqProxyModifiedStateUndoElement::New();
        undoElement->SetSession(source->getServer()->session());
        undoElement->MadeUnmodified(source);
        ADD_UNDO_ELEM(undoElement);
        undoElement->Delete();
        }
      }

    proxy->setModifiedState(pqProxy::UNMODIFIED);
    }

  this->updateInformationAndDomains();

  this->updateButtonState();

  if(this->View)
    {
    // re-render the view
    this->View->render();
    }

  emit this->applied();

  END_UNDO_SET();
}

void pqPropertiesPanel::reset()
{
  if(this->Proxy)
    {
    pqProxyPropertiesPanel *panel = this->ProxyPanels[this->Proxy];
    if(panel)
      {
      panel->reset();
      }

    this->Proxy->setModifiedState(pqProxy::UNMODIFIED);
    }

  this->updateInformationAndDomains();

  this->updateButtonState();
}

void pqPropertiesPanel::removeProxy(pqPipelineSource *proxy)
{
  if(!this->ProxyPanels.contains(proxy))
    {
    return;
    }

  pqProxyPropertiesPanel *panel = this->ProxyPanels.value(proxy, 0);
  if(panel)
    {
    delete panel;
    }

  this->ProxyPanels.remove(proxy);
}

void pqPropertiesPanel::deleteProxy()
{
  if(this->Proxy)
    {
    pqPipelineSource *source = qobject_cast<pqPipelineSource*>(this->Proxy);

    pqApplicationCore *core = pqApplicationCore::instance();
    BEGIN_UNDO_SET(QString("Delete %1").arg(source->getSMName()));
    core->getObjectBuilder()->destroy(source);
    END_UNDO_SET();

    // remove the proxy and its panel
    this->removeProxy(source);
    }
}

void pqPropertiesPanel::showHelp()
{
  if(this->Proxy)
    {
    this->helpRequested(this->Proxy->getProxy()->GetXMLName());
    this->helpRequested(this->Proxy->getProxy()->GetXMLGroup(),
                        this->Proxy->getProxy()->GetXMLName());
    }
}

void pqPropertiesPanel::handleConnectionChanged(pqPipelineSource *in,
                                                pqPipelineSource *out)
{
  Q_UNUSED(out);

  if(this->Proxy && this->Proxy == in)
    {
    this->updateButtonState();
    }
}

void pqPropertiesPanel::show(pqPipelineSource *source)
{
  pqDisplayPolicy *displayPolicy =
    pqApplicationCore::instance()->getDisplayPolicy();
  if(!displayPolicy)
    {
    qCritical() << "No display policy defined. Cannot create pending displays.";
    return;
    }

  // create representations for all output ports.
  for(int i = 0; i < source->getNumberOfOutputPorts(); i++)
    {
    pqDataRepresentation *repr =
      displayPolicy->createPreferredRepresentation(source->getOutputPort(i),
                                                   this->View,
                                                   false);
    if(!repr || !repr->getView())
      {
      continue;
      }

    pqView *reprView = repr->getView();
    pqPipelineFilter *filter = qobject_cast<pqPipelineFilter *>(source);
    if(filter)
      {
      filter->hideInputIfRequired(reprView);
      }
    reprView->render(); // these renders are collapsed
    }
}

void pqPropertiesPanel::updateButtonState()
{
  this->Ui->ApplyButton->setEnabled(false);
  this->Ui->ResetButton->setEnabled(false);

  foreach(pqProxy *proxy, this->ProxyPanels.keys())
    {
    if(!proxy)
      {
      continue;
      }

    if(proxy->modifiedState() == pqProxy::UNINITIALIZED)
      {
      if(this->DebugApplyButtonState)
        {
        qDebug() << "Enabling the Apply button because the " <<
                    (proxy->getProxy() ? proxy->getProxy()->GetXMLName() : "unknown") <<
                    "proxy is uninitialized";
        }

      this->Ui->ApplyButton->setEnabled(true);
      }
    else if(proxy->modifiedState() == pqProxy::MODIFIED)
      {
      if(this->DebugApplyButtonState)
        {
        qDebug() << "Enabling the Apply button because the " <<
                    (proxy->getProxy() ? proxy->getProxy()->GetXMLName() : "unknown") <<
                    "proxy is modified";
        }

      this->Ui->ApplyButton->setEnabled(true);
      this->Ui->ResetButton->setEnabled(true);
      }
    }
}

void pqPropertiesPanel::proxyPropertyChanged()
{
  if(this->DebugApplyButtonState)
    {
    qDebug() << "Proxy Property Changed";
    QObject *signalSender = this->sender();
    if(signalSender)
      {
      qDebug() << "  sender class: " << signalSender->metaObject()->className();

      pqPropertyWidget *senderWidget =
          qobject_cast<pqPropertyWidget *>(signalSender);
      if(senderWidget)
        {
        qDebug() << "  xml label: " << senderWidget->property()->GetXMLLabel();
        qDebug() << "  widget reason: " << senderWidget->reason();
        }
      }
    }

  if(this->Proxy->modifiedState() == pqProxy::UNMODIFIED)
    {
    this->Proxy->setModifiedState(pqProxy::MODIFIED);
    }

  if(pqPropertiesPanel::AutoApply)
    {
    QTimer::singleShot(pqPropertiesPanel::AutoApplyDelay, this, SLOT(apply()));
    }

  this->updateButtonState();
}

void pqPropertiesPanel::searchTextChanged(const QString &string)
{
  QList<pqPropertiesPanelItem> proxyItems;
  if(pqProxyPropertiesPanel *panel = this->ProxyPanels[this->Proxy])
    {
    proxyItems = panel->propertyWidgetItems();
    }

  QList<pqPropertiesPanelItem> representationItems = this->RepresentationPropertyItems;

  foreach(const pqPropertiesPanelItem &item, proxyItems + representationItems)
    {
    bool visible = isPanelItemVisible(item);

    if(visible)
      {
      visible = item.Name.contains(string, Qt::CaseInsensitive);
      }

    if(item.LabelWidget)
      {
      item.LabelWidget->setVisible(visible);
      }

    item.PropertyWidget->setVisible(visible);
    }
}

void pqPropertiesPanel::clearSearchText ()
{
  this->Ui->SearchLineEdit->clear ();
}

void pqPropertiesPanel::advancedButtonToggled(bool state)
{
  Q_UNUSED(state);

  QList<pqPropertiesPanelItem> proxyItems;
  if(pqProxyPropertiesPanel *panel = this->ProxyPanels[this->Proxy])
    {
    proxyItems = panel->propertyWidgetItems();
    }

  QList<pqPropertiesPanelItem> representationItems = this->RepresentationPropertyItems;

  foreach(const pqPropertiesPanelItem &item, proxyItems + representationItems)
    {
    bool visible = isPanelItemVisible(item);

    if(item.LabelWidget)
      {
      item.LabelWidget->setVisible(visible);
      }

    item.PropertyWidget->setVisible(visible);
    }

  // update the search results
  searchTextChanged(this->Ui->SearchLineEdit->text());
}

void pqPropertiesPanel::representationPropertyChanged(vtkObject *object, unsigned long event_, void *data_)
{
  Q_UNUSED(event_);
  Q_UNUSED(data_);

  if(!object)
    {
    return;
    }

  // update panels
  advancedButtonToggled(this->Ui->AdvancedButton->isChecked());
}

QList<pqPropertiesPanelItem> pqPropertiesPanel::createWidgetsForProxy(pqProxy *proxy)
{
  QList<pqPropertiesPanelItem> widgets;

  if(!proxy)
    {
    return widgets;
    }

  vtkSMProxy *smProxy = proxy->getProxy();

  // build set of all properties that are contained in a group
  QSet<vtkSMProperty *> groupProperties;
  for(size_t i = 0; i < smProxy->GetNumberOfPropertyGroups(); i++)
    {
    vtkSMPropertyGroup *group = smProxy->GetPropertyGroup(i);

    for(size_t j = 0; j < group->GetNumberOfProperties(); j++)
      {
      groupProperties.insert(group->GetProperty(static_cast<unsigned int>(j)));
      }

    if(QString(group->GetPanelVisibility()) == "never")
      {
      // skip property groups marked as never show
      if(this->DebugWidgetCreation)
        {
        qDebug() << "  - Group" << group->GetXMLLabel()
                 << "gets skipped because it has panel_visibility of \"never\"";
        }

      continue;
      }

    pqPropertyWidget *propertyWidget = 0;

    pqInterfaceTracker *interfaceTracker = pqApplicationCore::instance()->interfaceTracker();
    foreach(pqPropertyWidgetInterface *interface, interfaceTracker->interfaces<pqPropertyWidgetInterface *>())
      {
      propertyWidget = interface->createWidgetForPropertyGroup(smProxy, group);

      if(propertyWidget)
        {
        // stop if we successfully created a property widget
        break;
        }
      }

    if(propertyWidget)
      {
      // save record of the property widget
      pqPropertiesPanelItem item;
      item.Name = group->GetType();

      if(propertyWidget->showLabel())
        {
        item.LabelWidget = new QLabel(group->GetType());
        item.LabelWidget->setWordWrap(true);
        }
      else
        {
        item.LabelWidget = 0;
        }

      QString groupTypeName = group->GetType();
      groupTypeName.replace(" ", "");
      propertyWidget->setObjectName(groupTypeName);

      item.PropertyWidget = propertyWidget;
      item.IsAdvanced = QString(group->GetPanelVisibility()) == "advanced";
      item.Modified = true;

      this->connect(this, SIGNAL(viewChanged(pqView*)),
                    propertyWidget, SIGNAL(viewChanged(pqView*)));

      widgets.append(item);
      }
    }

  // iterate over each property, and create corresponding widgets
  vtkSMOrderedPropertyIterator *propertyIter = vtkSMOrderedPropertyIterator::New();
  propertyIter->SetProxy(smProxy);

  for(propertyIter->Begin(); !propertyIter->IsAtEnd(); propertyIter->Next())
    {
    const char *name = propertyIter->GetKey();
    vtkSMProperty *smProperty = propertyIter->GetProperty();

    if(smProperty->GetInformationOnly())
      {
      // skip information only properties
      if(this->DebugWidgetCreation)
        {
        qDebug() << "  -"
                 << proxy->getProxy()->GetPropertyName(smProperty)
                 << "(" << smProperty->GetXMLLabel() << ")"
                 << "gets skipped because it is an information only property";
        }

      continue;
      }
    else if(smProperty->GetIsInternal())
      {
      // skip internal properties
      if(this->DebugWidgetCreation)
        {
        qDebug() << "  -"
                 << proxy->getProxy()->GetPropertyName(smProperty)
                 << "(" << smProperty->GetXMLLabel() << ")"
                 << "gets skipped because it is an internal property";
        }

      continue;
      }
    else if(QString(smProperty->GetPanelVisibility()) == "never")
      {
      // skip properties marked as never show
      if(this->DebugWidgetCreation)
        {
        qDebug() << "  -"
                 << proxy->getProxy()->GetPropertyName(smProperty)
                 << "(" << smProperty->GetXMLLabel() << ")"
                 << "gets skipped because it has panel_visibility of \"never\"";
        }

      continue;
      }

    if(groupProperties.find(smProperty) != groupProperties.end())
      {
      // skip group properties
      continue;
      }

    // create property widget
    pqPropertyWidget *propertyWidget =
      this->createWidgetForProperty(smProperty, smProxy, this);

    if(propertyWidget)
      {
      QLabel *label = 0;
      if(propertyWidget->showLabel())
        {
        if(const char *xmlLabel = smProperty->GetXMLLabel())
          {
          label = new QLabel(xmlLabel);
          }
        else
          {
          label = new QLabel(name);
          }

        label->setWordWrap(true);
        }

      QString propertyKeyName = propertyIter->GetKey();
      propertyKeyName.replace(" ", "");
      propertyWidget->setObjectName(propertyKeyName);

      // store the property associated with the widget
      propertyWidget->setProperty(smProperty);

      // save record of the property widget and containing widget
      pqPropertiesPanelItem item;
      item.Name = name;
      item.LabelWidget = label;
      item.PropertyWidget = propertyWidget;
      item.IsAdvanced = QString(smProperty->GetPanelVisibility()) == "advanced";

      if(smProperty->GetPanelVisibilityDefaultForRepresentation())
        {
        item.DefaultVisibilityForRepresentations.append(
          smProperty->GetPanelVisibilityDefaultForRepresentation());
        }

      item.Modified = true;

      this->connect(this, SIGNAL(viewChanged(pqView*)),
                    propertyWidget, SIGNAL(viewChanged(pqView*)));

      // if debug widget creation is enabled then print the
      // reason why each property widget was created
      if(this->DebugWidgetCreation)
        {
        QString reason = item.PropertyWidget->reason();
        vtkSMProperty *originProperty = item.PropertyWidget->property();

        if(!reason.isEmpty())
          {
          qDebug() << "  -"
                   << proxy->getProxy()->GetPropertyName(originProperty)
                   << "(" << originProperty->GetXMLLabel() << ")"
                   << "gets a" << item.PropertyWidget->metaObject()->className()
                   << "containing a" << item.PropertyWidget->reason();
          }
        else
          {
          qDebug() << "  -"
                   << proxy->getProxy()->GetPropertyName(originProperty)
                   << "(" << originProperty->GetXMLLabel() << ")"
                   << "gets a" << item.PropertyWidget->metaObject()->className()
                   << "for an unknown reason";
          }
        }

      widgets.append(item);
      }
    }

  propertyIter->Delete();

  return widgets;
}

bool pqPropertiesPanel::isPanelItemVisible(const pqPropertiesPanelItem &item) const
{
  bool inAdvancedMode = this->Ui->AdvancedButton->isChecked();

  if(item.IsAdvanced)
    {
    // check for representation type
    if(this->Representation && !item.DefaultVisibilityForRepresentations.isEmpty())
      {
      const char *currentRepresentationName =
        vtkSMPropertyHelper(this->Representation->getProxy(), "Representation").GetAsString();

      if(currentRepresentationName &&
         item.DefaultVisibilityForRepresentations.contains(
           currentRepresentationName, Qt::CaseInsensitive))
        {
        return true;
        }
      }

    // return true if in advanced mode
    return inAdvancedMode;
    }
  else
    {
    return true;
    }
}

void pqPropertiesPanel::updateInformationAndDomains()
{
  if(!this->Proxy)
    {
    return;
    }

  vtkSMProxy *proxy = this->Proxy->getProxy();

  if(vtkSMSourceProxy *sourceProxy = vtkSMSourceProxy::SafeDownCast(proxy))
    {
    sourceProxy->UpdatePipelineInformation();
    }
  else
    {
    proxy->UpdatePropertyInformation();
    }

  vtkSMProperty *inputProperty = proxy->GetProperty("Input");
  if(inputProperty)
    {
    inputProperty->UpdateDependentDomains();
    }
}

void pqPropertiesPanel::setAutoApply(bool enabled)
{
  pqPropertiesPanel::AutoApply = enabled;
}

bool pqPropertiesPanel::autoApply()
{
  return pqPropertiesPanel::AutoApply;
}

void pqPropertiesPanel::setAutoApplyDelay(int delay)
{
  pqPropertiesPanel::AutoApplyDelay = delay;
}

int pqPropertiesPanel::autoApplyDelay()
{
  return pqPropertiesPanel::AutoApplyDelay;
}
