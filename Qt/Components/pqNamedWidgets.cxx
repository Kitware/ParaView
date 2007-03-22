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
#include <QLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QListWidget>
#include <QTreeWidget>
#include <QGroupBox>
#include <QSlider>
#include <QDoubleSpinBox>

// VTK includes

// ParaView Server Manager includes
#include "vtkSMProperty.h"
#include "vtkSMPropertyIterator.h"

// ParaView includes
#include "pqApplicationCore.h"
#include "pqListWidgetItemObject.h"
#include "pqObjectPanel.h"
#include "pqPipelineDisplay.h"
#include "pqPipelineModel.h"
#include "pqPipelineSource.h"
#include "pqPropertyManager.h"
#include "pqServerManagerModel.h"
#include "pqServerManagerObserver.h"
#include "pqSignalAdaptors.h"
#include "pqSMAdaptor.h"
#include "pqSMProxy.h"
#include "pqSMSignalAdaptors.h"
#include "pqTreeWidgetItemObject.h"
#include "pqComboBoxDomain.h"
#include "pqSpinBoxDomain.h"
#include "pqDoubleSpinBoxDomain.h"
#include "pqSliderDomain.h"
#include "pqFileChooserWidget.h"
#include "pqFieldSelectionAdaptor.h"
#include "pqProxySelectionWidget.h"

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
    }
  else if(pt == pqSMAdaptor::SELECTION)
    {
    // selections can be list widgets
    // for now, we're assuming selection domains don't change
    // if they do, we need to observe those changes
    QListWidget* listWidget = qobject_cast<QListWidget*>(object);
    QTreeWidget* treeWidget = qobject_cast<QTreeWidget*>(object);
    if(listWidget)
      {
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
      treeWidget->clear();
      QList<QVariant> sel_domain;
      sel_domain = pqSMAdaptor::getSelectionPropertyDomain(SMProperty);
      for(int j=0; j<sel_domain.size(); j++)
        {
        QList<QString> str;
        str.append(sel_domain[j].toString());
        pqTreeWidgetItemObject* item;
        item = new pqTreeWidgetItemObject(treeWidget, str);
        property_manager->registerLink(item, 
                                          "checked", 
                                          SIGNAL(checkedStateChanged(bool)),
                                          proxy, SMProperty, j);
        }
      }
    }
  else if(pt == pqSMAdaptor::PROXY)
    {
    QComboBox* comboBox = qobject_cast<QComboBox*>(object);
    if(comboBox)
      {
      // TODO: use pqComboBoxDomain
      QList<pqSMProxy> propertyDomain = 
        pqSMAdaptor::getProxyPropertyDomain(SMProperty);
      comboBox->clear();
      foreach(pqSMProxy v, propertyDomain)
        {
        pqPipelineSource* o = 
          pqServerManagerModel::instance()->getPQSource(v);
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
        w->setRenderModule(object_panel->renderModule());
        QObject::connect(parent, SIGNAL(renderModuleChanged(pqRenderViewModule*)),
                         w, SLOT(setRenderModule(pqRenderViewModule*)));
        QObject::connect(parent, SIGNAL(onaccept()), w, SLOT(accept()));
        QObject::connect(parent, SIGNAL(onreset()), w, SLOT(reset()));
        QObject::connect(parent, SIGNAL(onselect()), w, SLOT(select()));
        QObject::connect(parent, SIGNAL(ondeselect()), w, SLOT(deselect()));
        QObject::connect(w, SIGNAL(modified()), parent, SLOT(modified()));
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
    QDoubleSpinBox* doubleSpinBox = 
      qobject_cast<QDoubleSpinBox*>(object);
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
        QDoubleSpinBox* doubleSpinBox =
          qobject_cast<QDoubleSpinBox*>(propertyWidget);
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
      for(int ii=0; ii<treeWidget->topLevelItemCount(); ii++)
        {
        pqTreeWidgetItemObject* item;
        item = 
          static_cast<pqTreeWidgetItemObject*>(treeWidget->topLevelItem(ii));
        property_manager->unregisterLink(
          item, "checked", SIGNAL(checkedStateChanged(bool)),
          proxy, SMProperty, ii);
        }
      treeWidget->clear();
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
    QDoubleSpinBox* doubleSpinBox = 
      qobject_cast<QDoubleSpinBox*>(object);
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


