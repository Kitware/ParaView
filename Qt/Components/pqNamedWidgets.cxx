/*=========================================================================

   Program: ParaView
   Module:    pqNamedWidgets.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

// this include
#include "pqNamedWidgets.h"

// Qt includes
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSlider>
#include <QtDebug>
#include <QTextEdit>
#include <QTreeWidget>
#include <QLabel>
#include <QMetaObject>
#include <QMetaProperty>
#include <QHeaderView>

// VTK includes

// ParaView Server Manager includes
#include "vtkCollection.h"
#include "vtkPVXMLElement.h"
#include "vtkSmartPointer.h"
#include "vtkSMCompositeTreeDomain.h"
#include "vtkSMDomainIterator.h"
#include "vtkSMEnumerationDomain.h"
#include "vtkSMOrderedPropertyIterator.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMStringListDomain.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMSILDomain.h"

// ParaView includes
#include "pq3DWidget.h"
#include "pqApplicationCore.h"
#include "pqCollapsedGroup.h"
#include "pqComboBoxDomain.h"
#include "pqDoubleRangeWidget.h"
#include "pqFieldSelectionAdaptor.h"
#include "pqFileChooserWidget.h"
#include "pqIntRangeWidget.h"
#include "pqListWidgetItemObject.h"
#include "pqObjectBuilder.h"
#include "pqObjectPanel.h"
#include "pqPipelineFilter.h"
#include "pqPropertyManager.h"
#include "pqProxySelectionWidget.h"
#include "pqProxySILModel.h"
#include "pqSelectionInputWidget.h"
#include "pqServerManagerModel.h"
#include "pqServerManagerObserver.h"
#include "pqSignalAdaptorCompositeTreeWidget.h"
#include "pqSignalAdaptorSelectionTreeWidget.h"
#include "pqSignalAdaptors.h"
#include "pqSILModel.h"
#include "pqSILWidget.h"
#include "pqSMAdaptor.h"
#include "pqSMProxy.h"
#include "pqSMSignalAdaptors.h"
#include "pqTreeView.h"
#include "pqTreeWidget.h"
#include "pqTreeWidgetSelectionHelper.h"
#include "pqWidgetRangeDomain.h"

//-----------------------------------------------------------------------------
void pqNamedWidgets::link(QWidget* parent, pqSMProxy proxy, 
  pqPropertyManager* property_manager,
  const QStringList* exceptions/*=0*/)
{
  if(!parent || !proxy || !property_manager)
    {
    return;
    }
    
  vtkSMPropertyIterator *iter = proxy->NewPropertyIterator();
  for(iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
    // all property names with special characters are changed
    QString propertyName = iter->GetKey();
    if (exceptions && exceptions->contains(propertyName))
      {
      continue;
      }
    propertyName.replace(':', '_');
    
    // escape regex chars
    propertyName.replace(')', "\\)");
    propertyName.replace('(', "\\(");

    const QString regex = QString("^%1$|^%1_.*$").arg(propertyName);
    QList<QObject*> foundObjects = parent->findChildren<QObject*>(QRegExp(regex));
    for(int i=0; i<foundObjects.size(); i++)
      {
      QObject* foundObject = foundObjects[i];
      pqNamedWidgets::linkObject(foundObject, proxy, iter->GetKey(),
        property_manager);
      }
    }
  iter->Delete();

}

//-----------------------------------------------------------------------------
void pqNamedWidgets::linkObject(QObject* object, pqSMProxy proxy,
                         const QString& property, pqPropertyManager* property_manager)
{
  vtkSMProperty* SMProperty = proxy->GetProperty(property.toAscii().data());

  // update domains that we might ask for
  SMProperty->UpdateDependentDomains();

  pqSMAdaptor::PropertyType pt = pqSMAdaptor::getPropertyType(SMProperty);

  if(pt == pqSMAdaptor::MULTIPLE_ELEMENTS)
    {
    QWidget* propertyWidget = qobject_cast<QWidget*>(object);
    if(propertyWidget)
      {
      int index = -1;
      // get the index from the name, which should be at the end
      QString name = propertyWidget->objectName();
      QStringList split = name.split('_');
      if(split.size() > 1)
        {
        bool ok = false;
        index = split[split.size() - 1].toInt(&ok);
        if(!ok)
          {
          index = -1;
          }
        }
      if(index != -1)
        {
        QString userProperty, userSignal;
        if(pqNamedWidgets::propertyInformation(propertyWidget, userProperty, userSignal))
          {
          pqNamedWidgets::linkObject(propertyWidget, userProperty, userSignal,
            proxy, SMProperty, index, property_manager);
          }
        }
      }
    }
  else if(pt == pqSMAdaptor::ENUMERATION)
    {
    // enumerations can be combo boxes or check boxes
    QComboBox* comboBox = qobject_cast<QComboBox*>(object);
    if(comboBox)
      {
      pqComboBoxDomain* d0 = new pqComboBoxDomain(comboBox, SMProperty);
      d0->setObjectName("ComboBoxDomain");
      
      pqSignalAdaptorComboBox* adaptor = 
        new pqSignalAdaptorComboBox(comboBox);
      adaptor->setObjectName("ComboBoxAdaptor");
      property_manager->registerLink(
        adaptor, "currentText", SIGNAL(currentTextChanged(const QString&)),
        proxy, SMProperty);
      }
    else
      {
      QString userProperty, userSignal;
      if(pqNamedWidgets::propertyInformation(object, userProperty, userSignal))
        {
        pqNamedWidgets::linkObject(object, userProperty, userSignal,
          proxy, SMProperty, -1, property_manager);
        }
      }
    }
  else if(pt == pqSMAdaptor::SELECTION)
    {
    // selections can be list or tree widgets
    QListWidget* listWidget = qobject_cast<QListWidget*>(object);
    QTreeWidget* treeWidget = qobject_cast<QTreeWidget*>(object);
    if(listWidget)
      {
      // for now, we're assuming selection domains don't change
      // if they do, we need to observe those changes
      listWidget->clear();
      QList<QVariant> sel_domain = 
        pqSMAdaptor::getSelectionPropertyDomain(SMProperty);
      for(int j=0; j<sel_domain.size(); j++)
        {
        pqListWidgetItemObject* item = 
          new pqListWidgetItemObject(sel_domain[j].toString(), listWidget);
        property_manager->registerLink(
          item, "checked", SIGNAL(checkedStateChanged(bool)),
          proxy, SMProperty, j);
        }
      }
    else if(treeWidget)
      {
      pqSignalAdaptorSelectionTreeWidget* adaptor = 
        new pqSignalAdaptorSelectionTreeWidget(treeWidget, SMProperty);
      adaptor->setObjectName("SelectionTreeWidgetAdaptor");
      property_manager->registerLink(
        adaptor, "values", SIGNAL(valuesChanged()),
        proxy, SMProperty);
      }
    }
  else if(pt == pqSMAdaptor::PROXY)
    {
    QComboBox* comboBox = qobject_cast<QComboBox*>(object);
    if(comboBox)
      {
      pqServerManagerModel* smmodel = 
        pqApplicationCore::instance()->getServerManagerModel();
      // TODO: use pqComboBoxDomain
      QList<pqSMProxy> propertyDomain = 
        pqSMAdaptor::getProxyPropertyDomain(SMProperty);
      comboBox->clear();
      foreach(pqSMProxy v, propertyDomain)
        {
        pqPipelineSource* o = smmodel->findItem<pqPipelineSource*>(v);
        if(o)
          {
          comboBox->addItem(o->getSMName());
          }
        }
      pqSignalAdaptorComboBox* comboAdaptor = 
        new pqSignalAdaptorComboBox(comboBox);
      comboAdaptor->setObjectName("ComboBoxAdaptor");
      pqSignalAdaptorProxy* proxyAdaptor = 
        new pqSignalAdaptorProxy(comboAdaptor, "currentText", 
                                 SIGNAL(currentTextChanged(const QString&)));
      proxyAdaptor->setObjectName("ComboBoxProxyAdaptor");
      property_manager->registerLink(
        proxyAdaptor, "proxy", SIGNAL(proxyChanged(const QVariant&)),
        proxy, SMProperty);
      }

    pqSelectionInputWidget *selectWidget
      = qobject_cast<pqSelectionInputWidget*>(object);
    if (selectWidget)
      {
      QString userProperty, userSignal;
      if (pqNamedWidgets::propertyInformation(object, userProperty, userSignal))
        {
        pqNamedWidgets::linkObject(object, userProperty, userSignal,
                                   proxy, SMProperty, -1, property_manager);
        QObject::connect(property_manager, SIGNAL(aboutToAccept()),
          selectWidget, SLOT(preAccept()));
        QObject::connect(property_manager, SIGNAL(accepted()),
          selectWidget, SLOT(postAccept()));
        }
      }
    }
  else if (pt == pqSMAdaptor::PROXYSELECTION)
    {
    pqProxySelectionWidget* w = qobject_cast<pqProxySelectionWidget*>(object);
    if(w)
      {
      property_manager->registerLink(
        w, "proxy", SIGNAL(proxyChanged(pqSMProxy)),
        proxy, SMProperty);

      QWidget* parent = w->parentWidget();
      pqObjectPanel* object_panel = qobject_cast<pqObjectPanel*>(parent);
      if(object_panel)
        {
        w->setView(object_panel->view());
        QObject::connect(parent, SIGNAL(viewChanged(pqView*)),
                         w, SLOT(setView(pqView*)));
        QObject::connect(parent, SIGNAL(onaccept()), w, SLOT(accept()));
        QObject::connect(parent, SIGNAL(onreset()), w, SLOT(reset()));
        QObject::connect(parent, SIGNAL(onselect()), w, SLOT(select()));
        QObject::connect(parent, SIGNAL(ondeselect()), w, SLOT(deselect()));
        QObject::connect(w, SIGNAL(modified()), parent, SLOT(setModified()));
        }

      }
    }
  else if(pt == pqSMAdaptor::SINGLE_ELEMENT || pt == pqSMAdaptor::FILE_LIST)
    {
    QComboBox* comboBox = qobject_cast<QComboBox*>(object);
    QTextEdit* textEdit = qobject_cast<QTextEdit*>(object);

    if(comboBox)
      {
      // these combo boxes tend to be true/false combos
      pqComboBoxDomain* d0 = new pqComboBoxDomain(comboBox, SMProperty);
      d0->setObjectName("ComboBoxDomain");

      pqSignalAdaptorComboBox* adaptor = 
        new pqSignalAdaptorComboBox(comboBox);
      adaptor->setObjectName("ComboBoxAdaptor");
      property_manager->registerLink(
        adaptor, "currentText", SIGNAL(currentTextChanged(const QString&)),
        proxy, SMProperty);
      }
    else if(textEdit)
      {
      pqSignalAdaptorTextEdit *adaptor = 
        new pqSignalAdaptorTextEdit(textEdit);
      adaptor->setObjectName("TextEditAdaptor");          
      property_manager->registerLink(
        adaptor, "text", SIGNAL(textChanged()),
        proxy, SMProperty);
      }
    else
      {
      QString userProperty, userSignal;
      if(pqNamedWidgets::propertyInformation(object, userProperty, userSignal))
        {
        pqNamedWidgets::linkObject(object, userProperty, userSignal,
          proxy, SMProperty, -1, property_manager);
        }
      }
    }
  else if(pt == pqSMAdaptor::FIELD_SELECTION)
    {
    QComboBox* comboBox = qobject_cast<QComboBox*>(object);
    if(comboBox)
      {
      if(comboBox->objectName().contains(QRegExp("_mode$")))
        {
        pqComboBoxDomain* d0 = new pqComboBoxDomain(comboBox, SMProperty,
                                                    "field_list");
        d0->setObjectName("FieldModeDomain");

        pqSignalAdaptorComboBox* adaptor = 
          new pqSignalAdaptorComboBox(comboBox);
        adaptor->setObjectName("ComboBoxAdaptor");
        property_manager->registerLink(
          adaptor, "currentText", SIGNAL(currentTextChanged(const QString&)),
          proxy, SMProperty, 0);  // 0 means link mode for field selection
        }
      else if(comboBox->objectName().contains(QRegExp("_scalars$")))
        {
        pqComboBoxDomain* d0 = new pqComboBoxDomain(comboBox, SMProperty,
                                                    "array_list");
        d0->setObjectName("FieldScalarsDomain");

        pqSignalAdaptorComboBox* adaptor = 
          new pqSignalAdaptorComboBox(comboBox);
        adaptor->setObjectName("ComboBoxAdaptor");
        property_manager->registerLink(
          adaptor, "currentData", SIGNAL(currentTextChanged(const QString&)),
          proxy, SMProperty, 1);  // 1 means link scalar for field selection
        }
      else
        {
        // one combo for it all
        pqFieldSelectionAdaptor* adaptor = new
          pqFieldSelectionAdaptor(comboBox, SMProperty);
        adaptor->setObjectName("FieldSelectionAdaptor");
        property_manager->registerLink(
          adaptor, "attributeMode", SIGNAL(selectionChanged()),
          proxy, SMProperty, 0);
        property_manager->registerLink(
          adaptor, "scalar", SIGNAL(selectionChanged()),
          proxy, SMProperty, 1);
        }
      }
    }
  else if (pt == pqSMAdaptor::COMPOSITE_TREE)
    {
    QTreeWidget* tree = qobject_cast<QTreeWidget*>(object);
    if (tree)
      {
      pqSignalAdaptorCompositeTreeWidget* treeAdaptor = 
        new pqSignalAdaptorCompositeTreeWidget(tree,
          vtkSMIntVectorProperty::SafeDownCast(SMProperty));
      pqTreeWidgetSelectionHelper* helper = 
        new pqTreeWidgetSelectionHelper(tree);
      helper->setObjectName("CompositeTreeSelectionHelper");
      treeAdaptor->setObjectName("CompositeTreeAdaptor");
      property_manager->registerLink(
        treeAdaptor, "values", SIGNAL(valuesChanged()),
        proxy, SMProperty);
      }
    }
  else if (pt == pqSMAdaptor::SIL)
    {
    pqSILWidget* tree = qobject_cast<pqSILWidget*>(object);
    if (tree)
      {
      property_manager->registerLink(
        tree->activeModel(), "values", SIGNAL(valuesChanged()),
        proxy, SMProperty);
      }
    }
}

//-----------------------------------------------------------------------------
void pqNamedWidgets::unlink(QWidget* parent, pqSMProxy proxy, pqPropertyManager* property_manager)
{
  if(!parent || !proxy || !property_manager)
    {
    return;
    }

  vtkSMPropertyIterator *iter = proxy->NewPropertyIterator();
  for(iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
    // all property names with special characters are changed
    QString propertyName = iter->GetKey();
    propertyName.replace(':', '_');

    // escape regex chars
    propertyName.replace(')', "\\)");
    propertyName.replace('(', "\\(");

    const QString regex = QString("^%1$|^%1_.*$").arg(propertyName);
    QList<QObject*> foundObjects = parent->findChildren<QObject*>(QRegExp(regex));
    for(int i=0; i<foundObjects.size(); i++)
      {
      QObject* foundObject = foundObjects[i];
      pqNamedWidgets::unlinkObject(foundObject, proxy, iter->GetKey(),
        property_manager);
      }
    }
  iter->Delete();
  proxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void pqNamedWidgets::unlinkObject(QObject* object, pqSMProxy proxy,
                           const QString& property, pqPropertyManager* property_manager)
{

  vtkSMProperty* SMProperty = proxy->GetProperty(property.toAscii().data());
  
  pqSMAdaptor::PropertyType pt = pqSMAdaptor::getPropertyType(SMProperty);

  if(pt == pqSMAdaptor::MULTIPLE_ELEMENTS)
    {
    QWidget* propertyWidget = qobject_cast<QWidget*>(object);
    if(propertyWidget)
      {
      int index = -1;
      // get the index from the name
      QString name = propertyWidget->objectName();
      QStringList split = name.split('_');
      if(split.size() > 1)
        {
        bool ok = false;
        index = split[split.size() - 1].toInt(&ok);
        if(!ok)
          {
          index = -1;
          }
        }
      if(index != -1)
        {
        QString userProperty, userSignal;
        if(pqNamedWidgets::propertyInformation(propertyWidget, userProperty, userSignal))
          {
          pqNamedWidgets::unlinkObject(propertyWidget, userProperty, userSignal,
            proxy, SMProperty, index, property_manager);
          }
        }
      }
    }
  else if(pt == pqSMAdaptor::ENUMERATION)
    {
    // enumerations can be combo boxes or check boxes
    QComboBox* comboBox = qobject_cast<QComboBox*>(object);
    if(comboBox)
      {
      pqComboBoxDomain* d0 =
        comboBox->findChild<pqComboBoxDomain*>("ComboBoxDomain");
      if(d0)
        {
        delete d0;
        }

      pqSignalAdaptorComboBox* adaptor = 
        comboBox->findChild<pqSignalAdaptorComboBox*>("ComboBoxAdaptor");
      if(adaptor)
        {
        property_manager->unregisterLink(
          adaptor, "currentText", SIGNAL(currentTextChanged(const QString&)),
          proxy, SMProperty);
        delete adaptor;
        }
      }
    else
      {
      QString userProperty, userSignal;
      if(pqNamedWidgets::propertyInformation(object, userProperty, userSignal))
        {
        pqNamedWidgets::unlinkObject(object, userProperty, userSignal,
          proxy, SMProperty, -1, property_manager);
        }
      }
    }
  else if(pt == pqSMAdaptor::SELECTION)
    {
    // selections can be list widgets
    QListWidget* listWidget = qobject_cast<QListWidget*>(object);
    QTreeWidget* treeWidget = qobject_cast<QTreeWidget*>(object);
    if(listWidget)
      {
      for(int ii=0; ii<listWidget->count(); ii++)
        {
        pqListWidgetItemObject* item = 
          static_cast<pqListWidgetItemObject*>(listWidget->item(ii));
        property_manager->unregisterLink(
          item, "checked", SIGNAL(checkedStateChanged(bool)),
          proxy, SMProperty, ii);
        }
      listWidget->clear();
      }
    if(treeWidget)
      {
      QObject* adaptor = 
        treeWidget->findChild<pqSignalAdaptorComboBox*>("SelectionTreeWidgetAdaptor");
      property_manager->unregisterLink(
        adaptor, "values", SIGNAL(valuesChanged()),
        proxy, SMProperty);
      delete adaptor;
      }
    }
  else if(pt == pqSMAdaptor::PROXY)
    {
    QComboBox* comboBox = qobject_cast<QComboBox*>(object);
    if(comboBox)
      {
      QObject* comboAdaptor = 
        comboBox->findChild<pqSignalAdaptorComboBox*>("ComboBoxAdaptor");
      QObject* proxyAdaptor = 
        comboBox->findChild<pqSignalAdaptorComboBox*>("ComboBoxProxyAdaptor");
      property_manager->unregisterLink(
        proxyAdaptor, "proxy", SIGNAL(proxyChanged(const QVariant&)),
        proxy, SMProperty);
      delete proxyAdaptor;
      delete comboAdaptor;
      }
    }
  else if(pt == pqSMAdaptor::SINGLE_ELEMENT || pt == pqSMAdaptor::FILE_LIST)
    {
    QComboBox* comboBox = qobject_cast<QComboBox*>(object);
    QTextEdit* textEdit = qobject_cast<QTextEdit*>(object);
    if(comboBox)
      {
      pqComboBoxDomain* d0 =
        comboBox->findChild<pqComboBoxDomain*>("ComboBoxDomain");
      if(d0)
        {
        delete d0;
        }

      pqSignalAdaptorComboBox* adaptor = 
        comboBox->findChild<pqSignalAdaptorComboBox*>("ComboBoxAdaptor");
      property_manager->unregisterLink(
        adaptor, "currentText", SIGNAL(currentTextChanged(const QString&)),
        proxy, SMProperty);
      delete adaptor;
      }
    else if(textEdit)
      {
      pqSignalAdaptorTextEdit* adaptor = 
        textEdit->findChild<pqSignalAdaptorTextEdit*>("TextEditAdaptor");
      property_manager->unregisterLink(
        adaptor, "text", SIGNAL(textChanged()),
        proxy, SMProperty);
      }
    else
      {
      QString userProperty, userSignal;
      if(pqNamedWidgets::propertyInformation(object, userProperty, userSignal))
        {
        pqNamedWidgets::unlinkObject(object, userProperty, userSignal,
          proxy, SMProperty, -1, property_manager);
        }
      }
    }
  else if (pt == pqSMAdaptor::COMPOSITE_TREE)
    {
    QTreeWidget* tree = qobject_cast<QTreeWidget*>(object);
    if (tree)
      {
      pqSignalAdaptorCompositeTreeWidget* treeAdaptor = 
        tree->findChild<pqSignalAdaptorCompositeTreeWidget*>(
          "CompositeTreeAdaptor");
      property_manager->unregisterLink(
        treeAdaptor, "values", SIGNAL(valuesChanged()),
        proxy, SMProperty);
      delete treeAdaptor;

      pqTreeWidgetSelectionHelper* helper = 
        tree->findChild<pqTreeWidgetSelectionHelper*>(
          "CompositeTreeSelectionHelper");
      delete helper;
      }
    }
  else if(pt == pqSMAdaptor::FIELD_SELECTION)
    {
    QComboBox* comboBox = qobject_cast<QComboBox*>(object);
    if(comboBox)
      {
      if(comboBox->objectName().contains(QRegExp("_mode$")))
        {
        pqComboBoxDomain* d0 =
          comboBox->findChild<pqComboBoxDomain*>("FieldModeDomain");
        if(d0)
          {
          delete d0;
          }

        pqSignalAdaptorComboBox* adaptor = 
          comboBox->findChild<pqSignalAdaptorComboBox*>("ComboBoxAdaptor");
        if(adaptor)
          {
          property_manager->unregisterLink(
            adaptor, "currentText", SIGNAL(currentTextChanged(const QString&)),
            proxy, SMProperty, 0);  // 0 means link mode for field selection
          
          delete adaptor;
          }
        }
      else if(comboBox->objectName().contains(QRegExp("_scalars$")))
        {
        pqComboBoxDomain* d0 =
          comboBox->findChild<pqComboBoxDomain*>("FieldScalarsDomain");
        if(d0)
          {
          delete d0;
          }

        pqSignalAdaptorComboBox* adaptor = 
          comboBox->findChild<pqSignalAdaptorComboBox*>("ComboBoxAdaptor");
        if(adaptor)
          {
          property_manager->unregisterLink(
            adaptor, "currentText", SIGNAL(currentTextChanged(const QString&)),
            proxy, SMProperty, 1);  // 1 means link scalars for field selection
          
          delete adaptor;
          }
        }
      else
        {
        pqFieldSelectionAdaptor* adaptor = 
          comboBox->findChild<pqFieldSelectionAdaptor*>("FieldSelectionAdaptor");
        // one combo for it all
        property_manager->unregisterLink(
          adaptor, "attributeMode", SIGNAL(selectionChanged()),
          proxy, SMProperty, 0);
        property_manager->unregisterLink(
          adaptor, "scalar", SIGNAL(selectionChanged()),
          proxy, SMProperty, 1);
        
        delete adaptor;
        }
      }
    }
}

//-----------------------------------------------------------------------------
// Process hints and form complex widgets using hints.
static void processHints(QGridLayout* panelLayout,
                         vtkSMProxy* smProxy,
                         QStringList& propertiesToHide,
                         QStringList& propertiesToShow)
{
  // Obtain the list of input ports, we don't show any widgets for input ports.
  QList<const char*> inputPortNames = 
    pqPipelineFilter::getInputPorts(smProxy);
  foreach (const char* pname, inputPortNames)
    {
    propertiesToHide.push_back(pname);
    }

  // Get the hints for this proxy.
  // The hints may contain stuff about property groupping/layout
  // etc etc.
  vtkPVXMLElement* hints = smProxy->GetHints();
  if (!hints)
    {
    return;
    }

  // Check for any hints about whether to show or hide the widget associated
  // with a particular property.
  unsigned int numHints = hints->GetNumberOfNestedElements();
  for (unsigned int i = 0; i < numHints; i++)
    {
    vtkPVXMLElement *element = hints->GetNestedElement(i);
    if (QString("Property") == element->GetName())
      {
      QString propertyName = element->GetAttribute("name");
      int showProperty;
      if (element->GetScalarAttribute("show", &showProperty))
        {
        if (showProperty)
          {
          propertiesToShow.push_back(propertyName);
          }
        else
          {
          propertiesToHide.push_back(propertyName);
          }
        }
      }
    }

  pqObjectPanel* panel =
    qobject_cast<pqObjectPanel*>(panelLayout->parentWidget());

  // See if any properties are grouped into 3D widgets.
  QList<pq3DWidget*> widgets = pq3DWidget::createWidgets(smProxy, smProxy);  // TODO seems a bit odd to pass both the same
  if (widgets.size() == 0)
    {
    return;
    }
  int rowCount = 0;
  foreach (pq3DWidget* widget, widgets)
    {
    pqCollapsedGroup* group = 
      new pqCollapsedGroup(panel);
    group->setLayout(new QVBoxLayout(group));
    group->setTitle(widget->getHints()->GetAttribute("label"));
    widget->setParent(group);
    QObject::connect(panel, SIGNAL(viewChanged(pqView*)),
      widget,SLOT(setView(pqView*)));
    widget->setView(panel->view());
    widget->resetBounds();
    widget->reset();
    
    QObject::connect(panel, SIGNAL(onselect()), widget, SLOT(select()));
    QObject::connect(panel, SIGNAL(ondeselect()), widget, SLOT(deselect()));
    QObject::connect(panel, SIGNAL(onaccept()), widget, SLOT(accept()));
    QObject::connect(panel, SIGNAL(onreset()), widget, SLOT(reset()));
    QObject::connect(widget, SIGNAL(modified()),
                     panel, SLOT(setModified()));

    group->layout()->addWidget(widget);
    panelLayout->addWidget(group, rowCount++, 0, 1, 2);

    vtkSmartPointer<vtkCollection> elements = 
      vtkSmartPointer<vtkCollection>::New();
    vtkPVXMLElement* widgetHints = widget->getHints();
    widgetHints->GetElementsByName("Property", elements);
    for (int cc=0; cc < elements->GetNumberOfItems(); ++cc)
      {
      vtkPVXMLElement* child = vtkPVXMLElement::SafeDownCast(
        elements->GetItemAsObject(cc));
      if (!child)
        {
        continue;
        }
      propertiesToHide.push_back(child->GetAttribute("name"));
      }
    }
}

//-----------------------------------------------------------------------------
static void setupValidator(QLineEdit* lineEdit, QVariant::Type type)
{
  switch (type)
    {
  case QVariant::Double:
    lineEdit->setValidator(new QDoubleValidator(lineEdit));
    break;

  case QVariant::Int:
    lineEdit->setValidator(new QIntValidator(lineEdit));
    break;

  default:
    break;
    }
}

//-----------------------------------------------------------------------------
static QLabel* createPanelLabel(QWidget* parent, QString text, QString pname)
{
  QLabel* label = new QLabel(parent);
  label->setObjectName(QString("_labelFor")+pname);
  label->setText(text);
  label->setWordWrap(true);
  return label;
}

//-----------------------------------------------------------------------------
void pqNamedWidgets::createWidgets(QGridLayout* panelLayout, vtkSMProxy* pxy)
{
  bool row_streched = false; // when set, the extra setRowStretch() at the end
                             // is skipped
  int rowCount = 0;
  bool isCompoundProxy = pxy->IsA("vtkSMCompoundSourceProxy");

  // query for proxy properties, and create widgets
  vtkSMOrderedPropertyIterator *iter = vtkSMOrderedPropertyIterator::New();
  iter->SetProxy(pxy);

  int hasCellArrayStatus = 0;
  int hasPointArrayStatus = 0;

  // check for both CellArrayStatus & PointArrayStatus
  for(iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
    if(QString(iter->GetKey()) == QString("CellArrayStatus"))
      {
      hasCellArrayStatus = 1;
      }
    if(QString(iter->GetKey()) == QString("PointArrayStatus"))
      {
      hasPointArrayStatus = 1;
      }
    }

  int hasCellPointArrayStatus = 0;
  if(hasPointArrayStatus && hasCellArrayStatus)
    {
    hasCellPointArrayStatus = 1;
    }
  int cellPointArrayStatusWidgetAdded = 0;

  QStringList propertiesToHide;
  QStringList propertiesToShow;
  processHints(panelLayout, pxy, propertiesToHide, propertiesToShow);

  // Skip the filename property, unless the user has indicated that the filename
  // should be shown on the property panel.
  QString filenameProperty = pqObjectBuilder::getFileNamePropertyName(pxy);
  if (!filenameProperty.isEmpty() &&
    !propertiesToShow.contains(filenameProperty))
    {
    propertiesToHide.push_back(filenameProperty);
    }

  rowCount = panelLayout->rowCount();
  for(iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
    vtkSMProperty* SMProperty = iter->GetProperty();
    bool informationOnly = SMProperty->GetInformationOnly();

    QString propertyName = iter->GetKey();
    if (propertiesToHide.contains(propertyName))
      {
      continue;
      }
    propertyName.replace(':', '_');

    // When this proxy represents a custom filter, we want to use the property 
    //  names the user provided rather than the built in property labels:
    QString propertyLabel;
    if(isCompoundProxy)
      {
      propertyLabel = iter->GetKey();
      }
    else
      {
      propertyLabel = SMProperty->GetXMLLabel();
      }

    // skip information properties
    if((!propertiesToShow.contains(propertyName) && informationOnly) || 
        SMProperty->GetIsInternal())
      {
      continue;
      }

    // update domains we might ask for
    SMProperty->UpdateDependentDomains();

    pqSMAdaptor::PropertyType pt = pqSMAdaptor::getPropertyType(SMProperty);
    QList<QString> domainsTypes = pqSMAdaptor::getDomainTypes(SMProperty);

    vtkPVXMLElement *hints = SMProperty->GetHints();

    // skip input properties
    if(pt == pqSMAdaptor::PROXY || pt == pqSMAdaptor::PROXYLIST)
      {
      if(SMProperty == pxy->GetProperty("Input"))
        {
        continue;
        }
      }

    if(pt == pqSMAdaptor::PROXY)
      {
      if (hints && hints->FindNestedElementByName("SelectionInput"))
        {
        // Create a widget for grabbing the current selection.
        pqSelectionInputWidget *selectWidget
          = new pqSelectionInputWidget(panelLayout->parentWidget());
        selectWidget->setObjectName(propertyName);
        panelLayout->addWidget(selectWidget, rowCount, 0, 1, -1);
        rowCount++;
        }
      else
        {
        // create a combo box with list of proxies
        QComboBox* combo = new QComboBox(panelLayout->parentWidget());
        if(informationOnly)
          {
          combo->setEnabled(false);
          }
        combo->setObjectName(propertyName);
        QLabel* label = createPanelLabel(panelLayout->parentWidget(),
                                         propertyLabel,
                                         propertyName);
        panelLayout->addWidget(label, rowCount, 0, 1, 1);
        panelLayout->addWidget(combo, rowCount, 1, 1, 1);
        rowCount++;
        }
      }
    else if(pt == pqSMAdaptor::PROXYLIST)
      {
      // create a list of selections of proxies
      }
    else if (pt==pqSMAdaptor::PROXYSELECTION)
      {
      // for now, support only one level deep of proxy selections
      pqProxySelectionWidget* w = 
        new pqProxySelectionWidget(pxy, iter->GetKey(),
                                   propertyLabel,
                                   panelLayout->parentWidget());
      if(informationOnly)
        {
        w->setEnabled(false);
        }
      w->setObjectName(propertyName);
      panelLayout->addWidget(w, rowCount, 0, 1, 2);
      rowCount++;
      }
    else if(pt == pqSMAdaptor::ENUMERATION)
      {
      QVariant enum_property = pqSMAdaptor::getEnumerationProperty(SMProperty);
      if(enum_property.type() == QVariant::Bool)
        {
        // check box for true/false
        QCheckBox* check;
        check = new QCheckBox(propertyLabel, 
                              panelLayout->parentWidget());
        if(informationOnly)
          {
          check->setEnabled(false);
          }
        check->setObjectName(propertyName);
        panelLayout->addWidget(check, rowCount, 0, 1, 2);
        rowCount++;
        }
      else
        {
        vtkSMVectorProperty* vp = vtkSMVectorProperty::SafeDownCast(SMProperty);
        // Some enumeration properties, we add
        // ability to select mutliple elements. This is the case if the
        // SM property has repeat command flag set.
        if (vp && vp->GetRepeatCommand())
          {
          QTreeWidget* tw = new pqTreeWidget(panelLayout->parentWidget());
          if(informationOnly)
            {
            tw->setEnabled(false);
            }
          tw->setColumnCount(1);
          tw->setRootIsDecorated(false);
          QTreeWidgetItem* h = new QTreeWidgetItem();
          h->setData(0, Qt::DisplayRole, propertyLabel);
          tw->setHeaderItem(h);
          tw->setObjectName(propertyName);
          pqTreeWidgetSelectionHelper* helper = 
            new pqTreeWidgetSelectionHelper(tw);
          helper->setObjectName(QString("%1Helper").arg(propertyName));
          panelLayout->addWidget(tw, rowCount, 0, 1, 2);
          rowCount++;
          }
        else
          {
          // combo box with strings
          QComboBox* combo = new QComboBox(panelLayout->parentWidget());
          if(informationOnly)
            {
            combo->setEnabled(false);
            }
          combo->setObjectName(propertyName);
          QLabel* label = createPanelLabel(panelLayout->parentWidget(),
                                           propertyLabel,
                                           propertyName);
          panelLayout->addWidget(label, rowCount, 0, 1, 1);
          panelLayout->addWidget(combo, rowCount, 1, 1, 1);
          }
        rowCount++;
        }
      }
    else if(pt == pqSMAdaptor::SELECTION)
      {
      int doIt = 1;
      QString name = propertyName;
      QString header = propertyLabel;

      if((propertyName == QString("CellArrayStatus") ||
         propertyName == QString("PointArrayStatus")) &&
         hasCellPointArrayStatus == 1)
        {
        if(cellPointArrayStatusWidgetAdded == 1)
          {
          doIt = 0;
          }
        else
          {
          cellPointArrayStatusWidgetAdded = 1;
          name = "CellAndPointArrayStatus";
          header = "Cell/Point Array Status";
          }
        }
      if(doIt)
        {
        QList<QList<QVariant> > items;
        items = pqSMAdaptor::getSelectionProperty(SMProperty);
        QTreeWidget* tw = new pqTreeWidget(panelLayout->parentWidget());
        if(informationOnly)
          {
          tw->setEnabled(false);
          }
        tw->setColumnCount(1);
        tw->setRootIsDecorated(false);
        QTreeWidgetItem* h = new QTreeWidgetItem();
        h->setData(0, Qt::DisplayRole, header);
        tw->setHeaderItem(h);
        tw->setObjectName(name);
        pqTreeWidgetSelectionHelper* helper = 
          new pqTreeWidgetSelectionHelper(tw);
        helper->setObjectName(QString("%1Helper").arg(propertyName));
        panelLayout->addWidget(tw, rowCount, 0, 1, 2);
        rowCount++;
        }
      }
    else if(pt == pqSMAdaptor::SINGLE_ELEMENT)
      {
      QVariant elem_property = pqSMAdaptor::getElementProperty(SMProperty);
      QList<QVariant> propertyDomain;
      propertyDomain = pqSMAdaptor::getElementPropertyDomain(SMProperty);
      if(elem_property.type() == QVariant::String && propertyDomain.size())
        {
        // combo box with strings
        QComboBox* combo = new QComboBox(panelLayout->parentWidget());
        if(informationOnly)
          {
          combo->setEnabled(false);
          }
        row_streched = true;
        combo->setObjectName(propertyName);
        QLabel* label = createPanelLabel(panelLayout->parentWidget(),
                                         propertyLabel,
                                         propertyName);
        
        panelLayout->addWidget(label, rowCount, 0, 1, 1);
        panelLayout->addWidget(combo, rowCount, 1, 1, 1);
        rowCount++;
        }
      else if(elem_property.type() == QVariant::Int && 
              propertyDomain.size() == 2 &&
              propertyDomain[0].isValid() && propertyDomain[1].isValid())
        {
        QLabel* label = createPanelLabel(panelLayout->parentWidget(),
                                         propertyLabel,
                                         propertyName);

        pqIntRangeWidget* range;
        range = new pqIntRangeWidget(panelLayout->parentWidget());
        if(informationOnly)
          {
          range->setEnabled(false);
          }
        range->setObjectName(propertyName);
        range->setMinimum(propertyDomain[0].toInt());
        range->setMaximum(propertyDomain[1].toInt());
        panelLayout->addWidget(label, rowCount, 0, 1, 1);
        panelLayout->addWidget(range, rowCount, 1, 1, 1);
        range->show();
        rowCount++;
        }
      else if(elem_property.type() == QVariant::Double && 
              propertyDomain.size() == 2 && 
              propertyDomain[0].isValid() && propertyDomain[1].isValid() &&
              domainsTypes.contains("vtkSMDoubleRangeDomain"))
        {
        QLabel* label = createPanelLabel(panelLayout->parentWidget(),
                                         propertyLabel,
                                         propertyName);
        pqDoubleRangeWidget* range;
        range = new pqDoubleRangeWidget(panelLayout->parentWidget());
        if(informationOnly)
          {
          range->setEnabled(false);
          }
        range->setObjectName(propertyName);
        range->setMinimum(propertyDomain[0].toDouble());
        range->setMaximum(propertyDomain[1].toDouble());

        panelLayout->addWidget(label, rowCount, 0, 1, 1);
        panelLayout->addWidget(range, rowCount, 1, 1, 1);
        rowCount++;
        }
      else
        {
        //  what entry widget we should use for a stringvectorproperty
        bool multiLineString = false;
        if (hints)
          {
          vtkPVXMLElement* widgetHint = hints->FindNestedElementByName("Widget");
          if (widgetHint)
            {
            if (widgetHint->GetAttribute("type") &&
                strcmp(widgetHint->GetAttribute("type"), "multi_line") == 0)
              {
              multiLineString = true;
              }
            }
          }

        if (!multiLineString)
          {
          QLineEdit* lineEdit = new QLineEdit(panelLayout->parentWidget());
          if(informationOnly)
            {
            lineEdit->setEnabled(false);
            }
          lineEdit->setObjectName(propertyName);
          setupValidator(lineEdit, elem_property.type()); 
          
          QLabel* label = createPanelLabel(panelLayout->parentWidget(),
                                           propertyLabel,
                                           propertyName);
          
          panelLayout->addWidget(label, rowCount, 0, 1, 1);
          panelLayout->addWidget(lineEdit, rowCount, 1, 1, 1);
          rowCount++;
          }
        else
          {
          QTextEdit *textEdit = new QTextEdit(panelLayout->parentWidget());
          QFont textFont("Courier");
          textEdit->setFont(textFont);
          if(informationOnly)
            {
            textEdit->setEnabled(false);
            }
          textEdit->setObjectName(propertyName);
          textEdit->setAcceptRichText(false);
          // The default tab stop is too big
          textEdit->setTabStopWidth(textEdit->tabStopWidth()/4);
          // We don't want line wrap when editing, for example, a python script
          textEdit->setLineWrapMode(QTextEdit::NoWrap);
          
          QLabel* label = createPanelLabel(panelLayout->parentWidget(),
                                           QString(propertyLabel)+":",
                                           propertyName);
          
          panelLayout->addWidget(label, rowCount, 0, 1, 2);
          rowCount++;

          panelLayout->addWidget(textEdit, rowCount, 0, 1, 2);
          panelLayout->setRowStretch(rowCount, 1);
          row_streched = true;
          rowCount++;
          }
        }
      }
    else if(pt == pqSMAdaptor::MULTIPLE_ELEMENTS)
      {
      QList<QVariant> list_property;
      list_property = pqSMAdaptor::getMultipleElementProperty(SMProperty);
      QList<QList<QVariant> > domain;
      domain = pqSMAdaptor::getMultipleElementPropertyDomain(SMProperty);
      
      QLabel* label = createPanelLabel(panelLayout->parentWidget(),
                                       propertyLabel,
                                       propertyName);
      panelLayout->addWidget(label, rowCount, 0, 1, 1);
      QGridLayout* glayout = new QGridLayout;
      glayout->setSpacing(0);
      glayout->setObjectName(propertyName);
      panelLayout->addLayout(glayout, rowCount, 1, 1, 1);

      // we have a few different layouts
      static const int LayoutThreeByTwo = 0;
      static const int LayoutRow = 1;
      static const int LayoutColumn = 2;

      // let's peek at what property types and domains we have to determine
      // which layout to use
      int layoutMethod = LayoutRow;
      if(list_property.size() == 6)
        {
        layoutMethod = LayoutThreeByTwo;
        }
      else if(list_property.size() > 3)  // that many won't fit in a row
        {
        layoutMethod = LayoutColumn;
        }

      int i=0;
      foreach(QVariant v, list_property)
        {
        QLayoutItem* item = NULL;

        // create the widget(s)
        if( 0 /*v.type() == QVariant::String && 
           domain.size() && domain[i].size() */)  // link not supported yet
          {
          QComboBox* combo = new QComboBox(panelLayout->parentWidget());
          if(informationOnly)
            {
            combo->setEnabled(false);
            }
          combo->setObjectName(QString("%1_%2").arg(propertyName).arg(i));
          item = new QWidgetItem(combo);
          }
        else if(v.type() == QVariant::Int && 
           domain.size() && domain[i].size() == 2 &&
           domain[i][0].isValid() && domain[i][1].isValid())
          {
          int range[2];
          range[0] = domain[i][0].toInt();
          range[1] = domain[i][1].toInt();
          QSpinBox* spinBox;
          spinBox = new QSpinBox(panelLayout->parentWidget());
          if(informationOnly)
            {
            spinBox->setEnabled(false);
            }
          spinBox->setObjectName(QString("%1_%2").arg(propertyName).arg(i));
          spinBox->setRange(range[0], range[1]);
          item = new QWidgetItem(spinBox);
          }
        else if(v.type() == QVariant::Double && 
                domain.size() && domain[i].size() == 2 &&
                domain[i][0].isValid() && domain[i][1].isValid())
          {
          pqDoubleRangeWidget* range;
          range = new pqDoubleRangeWidget(panelLayout->parentWidget());
          if(informationOnly)
            {
            range->setEnabled(false);
            }
          range->setObjectName(QString("%1_%2").arg(propertyName).arg(i));
          range->setMinimum(domain[i][0].toDouble());
          range->setMaximum(domain[i][1].toDouble());
          item = new QWidgetItem(range);
          }
        else
          {
          QLineEdit* lineEdit = new QLineEdit(panelLayout->parentWidget());
          if(informationOnly)
            {
            lineEdit->setEnabled(false);
            }
          lineEdit->setObjectName(QString("%1_%2").arg(propertyName).arg(i));
          setupValidator(lineEdit, v.type()); 
          item = new QWidgetItem(lineEdit);
          }

        // insert the widget(s) in the layout
        if(layoutMethod == LayoutThreeByTwo)
          {
          glayout->addItem(item, i/2, i%2, 1, 1);
          }
        else if(layoutMethod == LayoutRow)
          {
          glayout->addItem(item, 0, i, 1, 1);
          }
        else if(layoutMethod == LayoutColumn)
          {
          glayout->addItem(item, i, 0, 1, 1);
          }


        // widgets in sub-layouts need help being shown
        QLayout* l = item->layout();
        if(l)
          {
          int count = l->count();
          for(int k=0; k<count; k++)
            {
            QLayoutItem* li = l->itemAt(k);
            if(li->widget())
              {
              li->widget()->show();
              }
            }
          }

        i++;
        }
        rowCount++;
      }
    else if(pt == pqSMAdaptor::FILE_LIST)
      {
      pqFileChooserWidget* chooser;
      chooser = new pqFileChooserWidget(panelLayout->parentWidget());
      if (informationOnly)
        {
        chooser->setEnabled(false);
        }

      // decide whether to allow multiple files
      chooser->setForceSingleFile((SMProperty->GetRepeatable() == 0));

      // If there's a hint on the property indicating that this property expects a
      // directory name, then, we will use the directory mode.
      if (SMProperty->IsA("vtkSMStringVectorProperty") && SMProperty->GetHints() && 
          SMProperty->GetHints()->FindNestedElementByName("UseDirectoryName"))
        {
        chooser->setUseDirectoryMode(1);
        }

      pqServerManagerModel* m =
        pqApplicationCore::instance()->getServerManagerModel();
      chooser->setServer(m->findServer(pxy->GetSession()));
      chooser->setObjectName(propertyName);
      QLabel* label = createPanelLabel(panelLayout->parentWidget(),
                                       propertyLabel,
                                       propertyName);

      panelLayout->addWidget(label, rowCount, 0, 1, 1);
      panelLayout->addWidget(chooser, rowCount, 1, 1, 1);
      rowCount++;
      }
    else if(pt == pqSMAdaptor::FIELD_SELECTION)
      {
      QLabel* label = createPanelLabel(panelLayout->parentWidget(),
                                       propertyLabel,
                                       propertyName);
      QComboBox* combo = new QComboBox(panelLayout->parentWidget());
      if(informationOnly)
        {
        combo->setEnabled(false);
        }
      combo->setObjectName(QString(propertyName));
      panelLayout->addWidget(label, rowCount, 0, 1, 1);
      panelLayout->addWidget(combo, rowCount, 1, 1, 1);
      rowCount++;
      }
    else if (pt == pqSMAdaptor::COMPOSITE_TREE)
      {
      QTreeWidget* tree = new QTreeWidget(panelLayout->parentWidget());
      if(informationOnly)
        {
        tree->setEnabled(false);
        }
      tree->setObjectName(propertyName);
        
      QTreeWidgetItem* header = new QTreeWidgetItem();
      header->setData(0, Qt::DisplayRole, propertyLabel);
      tree->setHeaderItem(header);
      panelLayout->addWidget(tree, rowCount, 0, 1, 2); 
      panelLayout->setRowStretch(rowCount, 1);
      row_streched = true;
      rowCount++;
      }
    else if (pt == pqSMAdaptor::SIL)
      {
      vtkSMSILDomain* silDomain = vtkSMSILDomain::SafeDownCast(
        SMProperty->GetDomain("array_list"));

      pqSILWidget* tree = new pqSILWidget( 
        silDomain->GetSubTree(), panelLayout->parentWidget());
      tree->setObjectName(propertyName);
      
      pqSILModel* silModel = new pqSILModel(tree);
      
      // FIXME: This needs to be automated, we want the model to automatically
      // fetch the SIL when the domain is updated.
      silModel->update(silDomain->GetSIL());
      tree->setModel(silModel);
      panelLayout->addWidget(tree, rowCount, 0, 1, 2); 
      panelLayout->setRowStretch(rowCount, 1);
      row_streched = true;
      rowCount++;
      }
    }
  iter->Delete();
  if (!row_streched)
    {
    panelLayout->setRowStretch(rowCount, 1);
    }
  panelLayout->invalidate();
}

//-----------------------------------------------------------------------------
bool pqNamedWidgets::propertyInformation(QObject* object, 
    QString& property, QString& signal)
{
  if(!object)
    {
    return false;
    }

  const QMetaObject* mo = object->metaObject();
  QMetaProperty UserProperty = mo->userProperty();

  if(UserProperty.isValid())
    {
    QString propertyName = UserProperty.name();
    QString signalName;
    signalName = QString("%1Changed").arg(propertyName);
    int numMethods = mo->methodCount();
    int signalIndex = -1;
    for(int i=0; signalIndex == -1 && i<numMethods; i++)
      {
      if(mo->method(i).methodType() == QMetaMethod::Signal)
        {
        if(QString(mo->method(i).signature()).startsWith(signalName))
          {
          signalIndex = i;
          }
        }
      }
    if(signalIndex != -1)
      {
      QString theSignal = SIGNAL(%1);
      signal = theSignal.arg(mo->method(signalIndex).signature());
      property = propertyName;
      return true;
      }
    }
    
  QAbstractButton* btn = qobject_cast<QAbstractButton*>(object);
  if(btn && btn->isCheckable())
     {
     property = "checked";
     signal = SIGNAL(toggled(bool));
     return true;
     }

  // QMetaMethod notifySignal () const is available in Qt 4.5 
  // This will make determining the signal name easier and more generic.
  // and the QGroupBox code will not be needed.
  QGroupBox* grp = qobject_cast<QGroupBox*>(object);
  if(grp && grp->isCheckable())
     {
     property = "checked";
     signal = SIGNAL(toggled(bool));
     return true;
     }

  return false;
}

//-----------------------------------------------------------------------------
void pqNamedWidgets::linkObject(QObject* o, const QString& property,
                       const QString& signal, pqSMProxy proxy,
                       vtkSMProperty* smProperty, int index,
                       pqPropertyManager* pm)
{
  pm->registerLink(o, property.toAscii().data(), 
    signal.toAscii().data(), proxy, smProperty, index);

  // if the widget has min or max property, hook it up too
  if(o->metaObject()->indexOfProperty("minimum") != -1 ||
     o->metaObject()->indexOfProperty("maximum") != -1)
    {
    QWidget* w = qobject_cast<QWidget*>(o);
    if(w)
      {
      pqWidgetRangeDomain* d0 = new
        pqWidgetRangeDomain(w, "minimum", "maximum", smProperty, index);
      d0->setObjectName("WidgetRangeDomain");
      }
    }
}

//-----------------------------------------------------------------------------
void pqNamedWidgets::unlinkObject(QObject* o, const QString& property,
                       const QString& signal, pqSMProxy proxy,
                       vtkSMProperty* smProperty, int index,
                       pqPropertyManager* pm)
{
  pqWidgetRangeDomain* d0 = 
    o->findChild<pqWidgetRangeDomain*>("WidgetRangeDomain");
  if(d0)
    {
    delete d0;
    }
  
  pm->unregisterLink(o, property.toAscii().data(), 
    signal.toAscii().data(), proxy, smProperty, index);
}

