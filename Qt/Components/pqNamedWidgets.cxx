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
#include "pqSignalAdaptorProxyList.h"
#include "pqSignalAdaptors.h"
#include "pqSMAdaptor.h"
#include "pqSMProxy.h"
#include "pqSMSignalAdaptors.h"
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

    // update domains that we might ask for
    SMProperty->UpdateDependentDomains();

    const QString regex = QString("^%1$|^%1:.*$").arg(iter->GetKey());
    QList<QObject*> foundObjects = parent->findChildren<QObject*>(QRegExp(regex));
    for(int i=0; i<foundObjects.size(); i++)
      {
      QObject* foundObject = foundObjects[i];
      pqSMAdaptor::PropertyType pt = pqSMAdaptor::getPropertyType(SMProperty);

      if(pt == pqSMAdaptor::MULTIPLE_ELEMENTS)
        {
        QWidget* propertyWidget = qobject_cast<QWidget*>(foundObject);
        if(propertyWidget)
          {
          int index = -1;
          // get the index from the name
          QString name = propertyWidget->objectName();
          QStringList split = name.split(':');
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
            QDoubleSpinBox* doubleSpinBox = qobject_cast<QDoubleSpinBox*>(foundObject);
            if(le)
              {
              property_manager->registerLink(
                le, "text", SIGNAL(textChanged(const QString&)),
                proxy, SMProperty, index);
              }
            else if(sl)
              {
              property_manager->registerLink(
                sl, "value", SIGNAL(valueChanged(int)),
                proxy, SMProperty, index);
              }
            else if(doubleSpinBox)
              {
              property_manager->registerLink(doubleSpinBox, "value", SIGNAL(valueChanged(double)),
                                                 proxy, SMProperty, index);
              }
            }
          }
        }
      else if(pt == pqSMAdaptor::ENUMERATION)
        {
        // enumerations can be combo boxes or check boxes
        QCheckBox* checkBox = qobject_cast<QCheckBox*>(foundObject);
        QComboBox* comboBox = qobject_cast<QComboBox*>(foundObject);
        QLineEdit* lineEdit = qobject_cast<QLineEdit*>(foundObject);
        if(checkBox)
          {
          property_manager->registerLink(
            checkBox, "checked", SIGNAL(toggled(bool)),
            proxy, SMProperty);
          }
        else if(comboBox)
          {
          QList<QVariant> domain = 
            pqSMAdaptor::getEnumerationPropertyDomain(SMProperty);
          comboBox->clear();
          foreach(QVariant v, domain)
            {
            comboBox->addItem(v.toString());
            }
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
        QListWidget* listWidget = qobject_cast<QListWidget*>(foundObject);
        QTreeWidget* treeWidget = qobject_cast<QTreeWidget*>(foundObject);
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
        QComboBox* comboBox = qobject_cast<QComboBox*>(foundObject);
        if(comboBox)
          {
          QList<pqSMProxy> propertyDomain = 
            pqSMAdaptor::getProxyPropertyDomain(SMProperty);
          comboBox->clear();
          foreach(pqSMProxy v, propertyDomain)
            {
            pqPipelineSource* o = 
              pqServerManagerModel::instance()->getPQSource(v);
            if(o)
              {
              comboBox->addItem(o->getProxyName());
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
        QComboBox* comboBox = qobject_cast<QComboBox*>(foundObject);
        QWidget* widgetFrame = parent->findChild<QWidget*>(
          QString("WidgetBox.%1").arg(iter->GetKey()));
        if (comboBox)
          {
          comboBox->clear();
          /*
          QList<pqSMProxy> propertyDomain = 
            pqSMAdaptor::getProxyPropertyDomain(SMProperty);
          foreach(pqSMProxy v, propertyDomain)
            {
            comboBox->addItem(v->GetXMLName());
            }
          pqSignalAdaptorComboBox* comboAdaptor = 
            new pqSignalAdaptorComboBox(comboBox);
          comboAdaptor->setObjectName("ComboBoxAdaptor");
          */
          pqProxy* pq_proxy =
            pqApplicationCore::instance()->getServerManagerModel()->getPQSource(proxy);
          pqSignalAdaptorProxyList* proxyAdaptor = 
            new pqSignalAdaptorProxyList(comboBox, pq_proxy, iter->GetKey());
          proxyAdaptor->setWidgetFrame(widgetFrame);
          pqObjectPanel* object_panel = qobject_cast<pqObjectPanel*>(parent);
          if (object_panel)
            {
            proxyAdaptor->setRenderModule(object_panel->getRenderModule());
            }
          QObject::connect(parent, SIGNAL(renderModuleChanged(pqRenderModule*)),
            proxyAdaptor, SLOT(setRenderModule(pqRenderModule*)));

          QObject::connect(parent, SIGNAL(onaccept()), proxyAdaptor, 
            SLOT(accept()));
          QObject::connect(parent, SIGNAL(onreset()), proxyAdaptor,
            SLOT(reset()));
          QObject::connect(parent, SIGNAL(onselect()), proxyAdaptor,
            SLOT(select()));
          QObject::connect(parent, SIGNAL(ondeselect()), proxyAdaptor,
            SLOT(deselect()));

          proxyAdaptor->setObjectName("ComboBoxProxyAdaptor");
          property_manager->registerLink(
            proxyAdaptor, "proxy", SIGNAL(proxyChanged(const QVariant&)),
            SIGNAL(modified()), proxy, SMProperty);
          }
        }
      else if(pt == pqSMAdaptor::SINGLE_ELEMENT)
        {
        QComboBox* comboBox = qobject_cast<QComboBox*>(foundObject);
        QLineEdit* lineEdit = qobject_cast<QLineEdit*>(foundObject);
        QSlider* slider = qobject_cast<QSlider*>(foundObject);
        QSpinBox* spinBox = qobject_cast<QSpinBox*>(foundObject);
        QDoubleSpinBox* doubleSpinBox = 
          qobject_cast<QDoubleSpinBox*>(foundObject);
        if(comboBox)
          {
          QList<QVariant> domain = 
            pqSMAdaptor::getElementPropertyDomain(SMProperty);
          comboBox->clear();
          for(int j=0; j<domain.size(); j++)
            {
            comboBox->addItem(domain[j].toString());
            }

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
        else if(slider)
          {
          property_manager->registerLink(
            slider, "value", SIGNAL(valueChanged(int)),
            proxy, SMProperty);
          }
        else if(doubleSpinBox)
          {
          property_manager->registerLink(
            doubleSpinBox, "value", SIGNAL(valueChanged(double)),
            proxy, SMProperty);
          }
        else if(spinBox)
          {
          property_manager->registerLink(
            spinBox, "value", SIGNAL(valueChanged(int)),
            proxy, SMProperty);
          }
        }
      else if(pt == pqSMAdaptor::FILE_LIST)
        {
        QLineEdit* lineEdit = qobject_cast<QLineEdit*>(foundObject);
        if(lineEdit)
          {
          property_manager->registerLink(
            lineEdit, "text", SIGNAL(textChanged(const QString&)),
            proxy, SMProperty);
          }
        }
      else if(pt == pqSMAdaptor::FIELD_SELECTION)
        {
        QComboBox* comboBox = qobject_cast<QComboBox*>(foundObject);
        if(comboBox)
          {
          if(comboBox->objectName().contains(QRegExp(":mode$")))
            {
            QList<QString> domain = 
              pqSMAdaptor::getFieldSelectionModeDomain(SMProperty);
            comboBox->clear();
            comboBox->addItems(domain);

            pqSignalAdaptorComboBox* adaptor = 
              new pqSignalAdaptorComboBox(comboBox);
            adaptor->setObjectName("ComboBoxAdaptor");
            property_manager->registerLink(
              adaptor, "currentText", SIGNAL(currentTextChanged(const QString&)),
              proxy, SMProperty, 0);  // 0 means link mode for field selection
            }
          if(comboBox->objectName().contains(QRegExp(":scalars$")))
            {
            QList<QString> domain = 
              pqSMAdaptor::getFieldSelectionScalarDomain(SMProperty);
            comboBox->clear();
            comboBox->addItems(domain);

            pqSignalAdaptorComboBox* adaptor = 
              new pqSignalAdaptorComboBox(comboBox);
            adaptor->setObjectName("ComboBoxAdaptor");
            property_manager->registerLink(
              adaptor, "currentText", SIGNAL(currentTextChanged(const QString&)),
              proxy, SMProperty, 1);  // 1 means link scalar for field selection
            }
          }
        }
      }
    }
  iter->Delete();

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

    const QString regex = QString("^%1$|^%1:.*$").arg(iter->GetKey());
    QList<QObject*> foundObjects = parent->findChildren<QObject*>(QRegExp(regex));
    for(int i=0; i<foundObjects.size(); i++)
      {
      QObject* foundObject = foundObjects[i];
      
      pqSMAdaptor::PropertyType pt = pqSMAdaptor::getPropertyType(SMProperty);

      if(pt == pqSMAdaptor::MULTIPLE_ELEMENTS)
        {
        QWidget* propertyWidget = qobject_cast<QWidget*>(foundObject);
        if(propertyWidget)
          {
          int index = -1;
          // get the index from the name
          QString name = propertyWidget->objectName();
          QStringList split = name.split(':');
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
            if(le)
              {
              property_manager->unregisterLink(
                le, "text", SIGNAL(textChanged(const QString&)),
                proxy, SMProperty, index);
              }
            else if(sl)
              {
              property_manager->unregisterLink(
                sl, "value", SIGNAL(valueChanged(int)),
                proxy, SMProperty, index);
              }
            else if(doubleSpinBox)
              {
              property_manager->unregisterLink(doubleSpinBox, 
                                                  "value", 
                                                  SIGNAL(valueChanged(double)),
                                                  proxy, SMProperty);
              }
            }
          }
        }
      else if(pt == pqSMAdaptor::ENUMERATION)
        {
        // enumerations can be combo boxes or check boxes
        QCheckBox* checkBox = qobject_cast<QCheckBox*>(foundObject);
        QComboBox* comboBox = qobject_cast<QComboBox*>(foundObject);
        QLineEdit* lineEdit = qobject_cast<QLineEdit*>(foundObject);
        if(checkBox)
          {
          property_manager->unregisterLink(
            checkBox, "checked", SIGNAL(toggled(bool)),
            proxy, SMProperty);
          }
        else if(comboBox)
          {
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
        }
      else if(pt == pqSMAdaptor::SELECTION)
        {
        // selections can be list widgets
        QListWidget* listWidget = qobject_cast<QListWidget*>(foundObject);
        QTreeWidget* treeWidget = qobject_cast<QTreeWidget*>(foundObject);
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
        QComboBox* comboBox = qobject_cast<QComboBox*>(foundObject);
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
        QComboBox* comboBox = qobject_cast<QComboBox*>(foundObject);
        QLineEdit* lineEdit = qobject_cast<QLineEdit*>(foundObject);
        QSlider* slider = qobject_cast<QSlider*>(foundObject);
        QDoubleSpinBox* doubleSpinBox = 
          qobject_cast<QDoubleSpinBox*>(foundObject);
        QSpinBox* spinBox = qobject_cast<QSpinBox*>(foundObject);
        if(comboBox)
          {
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
          property_manager->unregisterLink(
            slider, "value", SIGNAL(valueChanged(int)),
            proxy, SMProperty);
          }
        else if(doubleSpinBox)
          {
          property_manager->unregisterLink(
            doubleSpinBox, "value", SIGNAL(valueChanged(double)),
            proxy, SMProperty);
          }
        else if(spinBox)
          {
          property_manager->unregisterLink(
            spinBox, "value", SIGNAL(valueChanged(int)),
            proxy, SMProperty);
          }
        }
      else if(pt == pqSMAdaptor::FILE_LIST)
        {
        QLineEdit* lineEdit = qobject_cast<QLineEdit*>(foundObject);
        if(lineEdit)
          {
          property_manager->unregisterLink(
            lineEdit, "text", SIGNAL(textChanged(const QString&)),
            proxy, SMProperty);
          }
        }
      }
    }
  iter->Delete();
  proxy->UpdateVTKObjects();
}
