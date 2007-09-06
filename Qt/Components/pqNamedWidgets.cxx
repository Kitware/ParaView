/*=========================================================================

   Program: ParaView
   Module:    pqNamedWidgets.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

// VTK includes

// ParaView Server Manager includes
#include "vtkSMEnumerationDomain.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMStringListDomain.h"
#include "vtkPVXMLElement.h"
#include "vtkSMOrderedPropertyIterator.h"
#include "vtkSMDomainIterator.h"
#include "vtkSMVectorProperty.h"
#include "vtkSmartPointer.h"
#include "vtkCollection.h"

// ParaView includes
#include "pq3DWidget.h"
#include "pqApplicationCore.h"
#include "pqCollapsedGroup.h"
#include "pqComboBoxDomain.h"
#include "pqDoubleRangeWidgetDomain.h"
#include "pqDoubleRangeWidget.h"
#include "pqDoubleSpinBoxDomain.h"
#include "pqFieldSelectionAdaptor.h"
#include "pqFileChooserWidget.h"
#include "pqListWidgetItemObject.h"
#include "pqObjectPanel.h"
#include "pqPipelineFilter.h"
#include "pqPropertyManager.h"
#include "pqProxySelectionWidget.h"
#include "pqServerManagerModel.h"
#include "pqServerManagerObserver.h"
#include "pqSignalAdaptorSelectionTreeWidget.h"
#include "pqSignalAdaptors.h"
#include "pqSliderDomain.h"
#include "pqSMAdaptor.h"
#include "pqSMProxy.h"
#include "pqSMSignalAdaptors.h"
#include "pqSpinBoxDomain.h"
#include "pqTreeWidgetCheckHelper.h"
#include "pqTreeWidget.h"
#include "pqTreeWidgetItemObject.h"

void pqNamedWidgets::link(QWidget* parent, pqSMProxy proxy, pqPropertyManager* property_manager)
{
  if(!parent || !proxy || !property_manager)
    {
    return;
    }
    
  vtkSMPropertyIterator *iter = proxy->NewPropertyIterator();
  for(iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
    vtkSMProperty* SMProperty = iter->GetProperty();
    if(SMProperty->GetInformationOnly())
      {
      continue;
      }

    // all property names with special characters are changed
    QString propertyName = iter->GetKey();
    propertyName.replace(':', '_');
    propertyName.replace(' ', '_');

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
        QLineEdit* le = qobject_cast<QLineEdit*>(propertyWidget);
        QSlider* sl = qobject_cast<QSlider*>(propertyWidget);
        QDoubleSpinBox* doubleSpinBox = qobject_cast<QDoubleSpinBox*>(object);
        pqDoubleRangeWidget* doubleRange = qobject_cast<pqDoubleRangeWidget*>(object);
        QSpinBox* spinBox = qobject_cast<QSpinBox*>(object);
        if(le)
          {
          property_manager->registerLink(
            le, "text", SIGNAL(textChanged(const QString&)),
            proxy, SMProperty, index);
          }
        else if(sl)
          {
          pqSliderDomain* d0 = new
            pqSliderDomain(sl, SMProperty, index);
          d0->setObjectName("SliderDomain");
          property_manager->registerLink(
            sl, "value", SIGNAL(valueChanged(int)),
            proxy, SMProperty, index);
          }
        else if(doubleSpinBox)
          {
          pqDoubleSpinBoxDomain* d0 = new
            pqDoubleSpinBoxDomain(doubleSpinBox, SMProperty, index);
          d0->setObjectName("DoubleSpinBoxDomain");
          property_manager->registerLink(doubleSpinBox, "value", SIGNAL(valueChanged(double)),
                                             proxy, SMProperty, index);
          }
        else if(doubleRange)
          {
          pqDoubleRangeWidgetDomain* d0 = new
            pqDoubleRangeWidgetDomain(doubleRange, SMProperty, index);
          d0->setObjectName("DoubleRangeWidgetDomain");
          property_manager->registerLink(doubleRange, "value", SIGNAL(valueChanged(double)),
                                             proxy, SMProperty, index);
          }
        else if(spinBox)
          {
          pqSpinBoxDomain* d0 = new
            pqSpinBoxDomain(spinBox, SMProperty, index);
          d0->setObjectName("SpinBoxDomain");
          property_manager->registerLink(spinBox, "value",
                                         SIGNAL(valueChanged(int)),
                                         proxy, SMProperty, index);
          }
        }
      }
    }
  else if(pt == pqSMAdaptor::ENUMERATION)
    {
    // enumerations can be combo boxes or check boxes
    QCheckBox* checkBox = qobject_cast<QCheckBox*>(object);
    QComboBox* comboBox = qobject_cast<QComboBox*>(object);
    QLineEdit* lineEdit = qobject_cast<QLineEdit*>(object);
    pqTreeWidget* treeWidget = 
      qobject_cast<pqTreeWidget*>(object);
    if(checkBox)
      {
      property_manager->registerLink(
        checkBox, "checked", SIGNAL(toggled(bool)),
        proxy, SMProperty);
      }
    else if(comboBox)
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
    else if(lineEdit)
      {
      property_manager->registerLink(
        lineEdit, "text", SIGNAL(textChanged(const QString&)),
        proxy, SMProperty);
      }
    else if (treeWidget)
      {
      Q_ASSERT("invalid tree widget for enumeration\n" == 0);
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
  else if(pt == pqSMAdaptor::SINGLE_ELEMENT)
    {
    QComboBox* comboBox = qobject_cast<QComboBox*>(object);
    QLineEdit* lineEdit = qobject_cast<QLineEdit*>(object);
    QTextEdit* textEdit = qobject_cast<QTextEdit*>(object);
    QSlider* slider = qobject_cast<QSlider*>(object);
    QSpinBox* spinBox = qobject_cast<QSpinBox*>(object);
    QDoubleSpinBox* doubleSpinBox = qobject_cast<QDoubleSpinBox*>(object);
    pqDoubleRangeWidget* doubleRange = qobject_cast<pqDoubleRangeWidget*>(object);
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
    else if(lineEdit)
      {
      property_manager->registerLink(
        lineEdit, "text", SIGNAL(textChanged(const QString&)),
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
    else if(slider)
      {
      pqSliderDomain* d0 = new
        pqSliderDomain(slider, SMProperty);
      d0->setObjectName("SliderDomain");
      property_manager->registerLink(
        slider, "value", SIGNAL(valueChanged(int)),
        proxy, SMProperty);
      }
    else if(doubleSpinBox)
      {
      pqDoubleSpinBoxDomain* d0 = new
        pqDoubleSpinBoxDomain(doubleSpinBox, SMProperty);
      d0->setObjectName("DoubleSpinBoxDomain");
      property_manager->registerLink(
        doubleSpinBox, "value", SIGNAL(valueChanged(double)),
        proxy, SMProperty);
      }
    else if(doubleRange)
      {
      pqDoubleRangeWidgetDomain* d0 = new
        pqDoubleRangeWidgetDomain(doubleRange, SMProperty);
      d0->setObjectName("DoubleRangeWidgetDomain");
      property_manager->registerLink(
        doubleRange, "value", SIGNAL(valueChanged(double)),
        proxy, SMProperty);
      }
    else if(spinBox)
      {
      pqSpinBoxDomain* d0 = new
        pqSpinBoxDomain(spinBox, SMProperty);
      d0->setObjectName("SpinBoxDomain");

      property_manager->registerLink(
        spinBox, "value", SIGNAL(valueChanged(int)),
        proxy, SMProperty);
      }
    }
  else if(pt == pqSMAdaptor::FILE_LIST)
    {
    QLineEdit* lineEdit = qobject_cast<QLineEdit*>(object);
    pqFileChooserWidget* chooser = 
           qobject_cast<pqFileChooserWidget*>(object);
    if(lineEdit)
      {
      property_manager->registerLink(
        lineEdit, "text", SIGNAL(textChanged(const QString&)),
        proxy, SMProperty);
      }
    else if(chooser)
      {
      property_manager->registerLink(
        chooser, "Filename", SIGNAL(filenameChanged(const QString&)),
        proxy, SMProperty);
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
          adaptor, "currentText", SIGNAL(currentTextChanged(const QString&)),
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
}


void pqNamedWidgets::unlink(QWidget* parent, pqSMProxy proxy, pqPropertyManager* property_manager)
{
  if(!parent || !proxy || !property_manager)
    {
    return;
    }

  vtkSMPropertyIterator *iter = proxy->NewPropertyIterator();
  for(iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
    vtkSMProperty* SMProperty = iter->GetProperty();
    if(SMProperty->GetInformationOnly())
      {
      continue;
      }

    // all property names with special characters are changed
    QString propertyName = iter->GetKey();
    propertyName.replace(':', '_');
    propertyName.replace(' ', '_');
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
        QLineEdit* le = qobject_cast<QLineEdit*>(propertyWidget);
        QSlider* sl = qobject_cast<QSlider*>(propertyWidget);
        QDoubleSpinBox* doubleSpinBox = qobject_cast<QDoubleSpinBox*>(propertyWidget);
        pqDoubleRangeWidget* doubleRange = qobject_cast<pqDoubleRangeWidget*>(propertyWidget);
        QSpinBox* spinBox = qobject_cast<QSpinBox*>(propertyWidget);
        if(le)
          {
          property_manager->unregisterLink(
            le, "text", SIGNAL(textChanged(const QString&)),
            proxy, SMProperty, index);
          }
        else if(sl)
          {
          pqSliderDomain* d0 = 
            sl->findChild<pqSliderDomain*>("SliderDomain");
          if(d0)
            {
            delete d0;
            }
          property_manager->unregisterLink(
            sl, "value", SIGNAL(valueChanged(int)),
            proxy, SMProperty, index);
          }
        else if(doubleSpinBox)
          {
          pqDoubleSpinBoxDomain* d0 = 
            doubleSpinBox->findChild<pqDoubleSpinBoxDomain*>("DoubleSpinBoxDomain");
          if(d0)
            {
            delete d0;
            }
          property_manager->unregisterLink(doubleSpinBox, 
                                              "value", 
                                              SIGNAL(valueChanged(double)),
                                              proxy, SMProperty);
          }
        else if(doubleRange)
          {
          pqDoubleRangeWidgetDomain* d0 = 
            doubleRange->findChild<pqDoubleRangeWidgetDomain*>("DoubleRangeWidgetDomain");
          if(d0)
            {
            delete d0;
            }
          property_manager->unregisterLink(doubleRange, 
                                              "value", 
                                              SIGNAL(valueChanged(double)),
                                              proxy, SMProperty);
          }
        else if(spinBox)
          {
          pqSpinBoxDomain* d0 = 
            spinBox->findChild<pqSpinBoxDomain*>("SpinBoxDomain");
          if(d0)
            {
            delete d0;
            }
          property_manager->unregisterLink(spinBox, 
                                           "value", 
                                           SIGNAL(valueChanged(int)),
                                           proxy, SMProperty);
          }
        }
      }
    }
  else if(pt == pqSMAdaptor::ENUMERATION)
    {
    // enumerations can be combo boxes or check boxes
    QCheckBox* checkBox = qobject_cast<QCheckBox*>(object);
    QComboBox* comboBox = qobject_cast<QComboBox*>(object);
    QLineEdit* lineEdit = qobject_cast<QLineEdit*>(object);
    pqTreeWidget* treeWidget = 
      qobject_cast<pqTreeWidget*>(object);
    if(checkBox)
      {
      property_manager->unregisterLink(
        checkBox, "checked", SIGNAL(toggled(bool)),
        proxy, SMProperty);
      }
    else if(comboBox)
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
    else if(lineEdit)
      {
      property_manager->unregisterLink(
        lineEdit, "text", SIGNAL(textChanged(const QString&)),
        proxy, SMProperty);
      }
    else if (treeWidget)
      {
      Q_ASSERT("invalid tree widget for enumeration\n" == 0);
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
  else if(pt == pqSMAdaptor::SINGLE_ELEMENT)
    {
    QComboBox* comboBox = qobject_cast<QComboBox*>(object);
    QLineEdit* lineEdit = qobject_cast<QLineEdit*>(object);
    QSlider* slider = qobject_cast<QSlider*>(object);
    QDoubleSpinBox* doubleSpinBox = qobject_cast<QDoubleSpinBox*>(object);
    pqDoubleRangeWidget* doubleRange = qobject_cast<pqDoubleRangeWidget*>(object);
    QSpinBox* spinBox = qobject_cast<QSpinBox*>(object);
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
    else if(lineEdit)
      {
      property_manager->unregisterLink(
        lineEdit, "text", SIGNAL(textChanged(const QString&)),
        proxy, SMProperty);
      }
    else if(slider)
      {
      pqSliderDomain* d0 = 
        slider->findChild<pqSliderDomain*>("SliderDomain");
      if(d0)
        {
        delete d0;
        }
      property_manager->unregisterLink(
        slider, "value", SIGNAL(valueChanged(int)),
        proxy, SMProperty);
      }
    else if(doubleSpinBox)
      {
      pqDoubleSpinBoxDomain* d0 = 
        doubleSpinBox->findChild<pqDoubleSpinBoxDomain*>("DoubleSpinBoxDomain");
      if(d0)
        {
        delete d0;
        }
      property_manager->unregisterLink(
        doubleSpinBox, "value", SIGNAL(valueChanged(double)),
        proxy, SMProperty);
      }
    else if(doubleRange)
      {
      pqDoubleRangeWidgetDomain* d0 = 
        doubleRange->findChild<pqDoubleRangeWidgetDomain*>("DoubleRangeWidgetDomain");
      if(d0)
        {
        delete d0;
        }
      property_manager->unregisterLink(
        doubleRange, "value", SIGNAL(valueChanged(double)),
        proxy, SMProperty);
      }
    else if(spinBox)
      {
      pqSpinBoxDomain* d0 = 
        spinBox->findChild<pqSpinBoxDomain*>("SpinBoxDomain");
      if(d0)
        {
        delete d0;
        }
      property_manager->unregisterLink(
        spinBox, "value", SIGNAL(valueChanged(int)),
        proxy, SMProperty);
      }
    }
  else if(pt == pqSMAdaptor::FILE_LIST)
    {
    QLineEdit* lineEdit = qobject_cast<QLineEdit*>(object);
    pqFileChooserWidget* chooser = 
           qobject_cast<pqFileChooserWidget*>(object);
    if(lineEdit)
      {
      property_manager->unregisterLink(
        lineEdit, "text", SIGNAL(textChanged(const QString&)),
        proxy, SMProperty);
      }
    else if(chooser)
      {
      property_manager->unregisterLink(
        chooser, "Filename", SIGNAL(filenameChanged(const QString&)),
        proxy, SMProperty);
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

static QLabel* createPanelLabel(QWidget* parent, QString text, QString pname)
{
  QLabel* label = new QLabel(parent);
  label->setObjectName(pname+QString("_label"));
  label->setText(text);
  label->setWordWrap(true);
  return label;
}

void pqNamedWidgets::createWidgets(QGridLayout* panelLayout,
                                               vtkSMProxy* pxy)
{
  int rowCount = 0;
  int skippedFirstFileProperty = 0;
  bool isCompoundProxy = pxy->IsA("vtkSMCompoundProxy");
  bool isSourceProxy = !pxy->GetProperty("Input");

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
  rowCount = panelLayout->rowCount();
  for(iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
    vtkSMProperty* SMProperty = iter->GetProperty();
    QString propertyName = iter->GetKey();
    if (propertiesToHide.contains(propertyName))
      {
      continue;
      }
    propertyName.replace(':', '_');
    propertyName.replace(' ', '_');

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
    if(SMProperty->GetInformationOnly() || SMProperty->GetIsInternal())
      {
      continue;
      }

    if (!propertiesToShow.contains(propertyName))
      {
      if (isSourceProxy &&
          SMProperty->IsA("vtkSMStringVectorProperty") && 
          !skippedFirstFileProperty)
        {
        // Do not show the first file property. We do not allow changing of
        // the main filename from the gui. The user has to create another
        // instance of the reader and reconnect the pipeline.
        vtkSmartPointer<vtkSMDomainIterator> diter;
        diter.TakeReference(SMProperty->NewDomainIterator());
        diter->Begin();
        while(!diter->IsAtEnd())
          {
          if (diter->GetDomain()->IsA("vtkSMFileListDomain"))
            {
            break;
            }
          diter->Next();
          }
        if (!diter->IsAtEnd())
          {
          skippedFirstFileProperty = 1;
          continue;
          }
        }
      }

    // update domains we might ask for
    SMProperty->UpdateDependentDomains();

    pqSMAdaptor::PropertyType pt = pqSMAdaptor::getPropertyType(SMProperty);
    QList<QString> domainsTypes = pqSMAdaptor::getDomainTypes(SMProperty);

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
      // create a combo box with list of proxies
      QComboBox* combo = new QComboBox(panelLayout->parentWidget());
      combo->setObjectName(propertyName);
      QLabel* label = createPanelLabel(panelLayout->parentWidget(),
                                       propertyLabel,
                                       propertyName);
      panelLayout->addWidget(label, rowCount, 0, 1, 1);
      panelLayout->addWidget(combo, rowCount, 1, 1, 1);
      rowCount++;
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
          tw->setColumnCount(1);
          tw->setRootIsDecorated(false);
          QTreeWidgetItem* h = new QTreeWidgetItem();
          h->setData(0, Qt::DisplayRole, propertyLabel);
          tw->setHeaderItem(h);
          tw->setObjectName(propertyName);
          new pqTreeWidgetCheckHelper(tw, 0, tw);
          panelLayout->addWidget(tw, rowCount, 0, 1, 2);
          rowCount++;
          }
        else
          {
          // combo box with strings
          QComboBox* combo = new QComboBox(panelLayout->parentWidget());
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
        tw->setColumnCount(1);
        tw->setRootIsDecorated(false);
        QTreeWidgetItem* h = new QTreeWidgetItem();
        h->setData(0, Qt::DisplayRole, header);
        tw->setHeaderItem(h);
        tw->setObjectName(name);
        new pqTreeWidgetCheckHelper(tw, 0, tw);
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
        QSlider* slider;
        slider = new QSlider(Qt::Horizontal, panelLayout->parentWidget());
        slider->setObjectName(QString(propertyName) + "_Slider");
        slider->setRange(propertyDomain[0].toInt(), propertyDomain[1].toInt());

        QLineEdit* lineEdit = new QLineEdit(panelLayout->parentWidget());
        lineEdit->setObjectName(propertyName);
        setupValidator(lineEdit, elem_property.type()); 
        panelLayout->addWidget(label, rowCount, 0, 1, 1);
        QHBoxLayout* hlayout = new QHBoxLayout;
        hlayout->addWidget(slider);
        hlayout->addWidget(lineEdit);
        panelLayout->addLayout(hlayout, rowCount, 1, 1, 1);
        slider->show();
        lineEdit->show();
        rowCount++;
        }
      else if(elem_property.type() == QVariant::Double && 
              propertyDomain.size() == 2 && 
              propertyDomain[0].isValid() && propertyDomain[1].isValid() &&
              domainsTypes.contains("vtkSMDoubleRangeDomain"))
        {
        /*
        double range[2];
        range[0] = propertyDomain[0].toDouble();
        range[1] = propertyDomain[1].toDouble();
        */
        QLabel* label = createPanelLabel(panelLayout->parentWidget(),
                                         propertyLabel,
                                         propertyName);
        /*
        QDoubleSpinBox* spinBox;
        spinBox = new QDoubleSpinBox(panelLayout->parentWidget());
        spinBox->setObjectName(propertyName);
        spinBox->setRange(range[0], range[1]);
        spinBox->setSingleStep((range[1] - range[0]) / 20.0);
        */
        pqDoubleRangeWidget* range;
        range = new pqDoubleRangeWidget(panelLayout->parentWidget());
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
        vtkPVXMLElement* hints = SMProperty->GetHints();
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
          spinBox->setObjectName(QString("%1_%2").arg(propertyName).arg(i));
          spinBox->setRange(range[0], range[1]);
          item = new QWidgetItem(spinBox);
          }
        else if(v.type() == QVariant::Double && 
                domain.size() && domain[i].size() == 2 &&
                domain[i][0].isValid() && domain[i][1].isValid())
          {
          /*
          double range[2];
          range[0] = domain[i][0].toDouble();
          range[1] = domain[i][1].toDouble();
          QDoubleSpinBox* spinBox;
          spinBox = new QDoubleSpinBox(panelLayout->parentWidget());
          spinBox->setObjectName(QString("%1_%2").arg(propertyName).arg(i));
          spinBox->setRange(range[0], range[1]);
          spinBox->setSingleStep((range[1] - range[0]) / 20.0);
          */
          pqDoubleRangeWidget* range;
          range = new pqDoubleRangeWidget(panelLayout->parentWidget());
          range->setObjectName(QString("%1_%2").arg(propertyName).arg(i));
          range->setMinimum(domain[i][0].toDouble());
          range->setMaximum(domain[i][1].toDouble());
          item = new QWidgetItem(range);
          }
        else
          {
          QLineEdit* lineEdit = new QLineEdit(panelLayout->parentWidget());
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
      pqServerManagerModel* m =
        pqApplicationCore::instance()->getServerManagerModel();
      chooser->setServer(m->findServer(pxy->GetConnectionID()));
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
                                       "Scalars",
                                       propertyName);
      QComboBox* combo = new QComboBox(panelLayout->parentWidget());
      combo->setObjectName(QString(propertyName));
      panelLayout->addWidget(label, rowCount, 0, 1, 1);
      panelLayout->addWidget(combo, rowCount, 1, 1, 1);
      rowCount++;
      }
    }
  iter->Delete();
  panelLayout->addItem(new QSpacerItem(0,0,
                                       QSizePolicy::Expanding,
                                       QSizePolicy::Expanding), 
                       rowCount, 0, 1, 2);
  panelLayout->invalidate();
}


