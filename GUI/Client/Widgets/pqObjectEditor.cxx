/*=========================================================================

   Program:   ParaQ
   Module:    pqObjectEditor.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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
#include "pqObjectEditor.h"

// Qt includes
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>
#include <QListWidget>
#include <QScrollArea>

// VTK includes
#include "QVTKWidget.h"

// paraview includes
#include "vtkSMPropertyIterator.h"
#include "vtkSMProperty.h"

// paraq includes
#include "pqSMAdaptor.h"
#include "pqSMProxy.h"
#include "pqPipelineData.h"
#include "pqPipelineObject.h"

/// constructor
pqObjectEditor::pqObjectEditor(QWidget* p)
  : QWidget(p), Proxy(NULL)
{
  QBoxLayout* mainlayout = new QVBoxLayout(this);
  mainlayout->setMargin(0);
  
  QBoxLayout* buttonlayout = new QHBoxLayout();
  mainlayout->addLayout(buttonlayout);

  QScrollArea* qscroll = new QScrollArea(this);
  qscroll->setObjectName("Object Editor Scroll");
  QWidget* w = new QWidget;
  w->setObjectName("Object Editor Panel");
  w->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
  qscroll->setWidgetResizable(true);
  qscroll->setWidget(w);
  this->PanelLayout = new QGridLayout(w);
  mainlayout->addWidget(qscroll);
  
  QPushButton* acceptButton = new QPushButton(this);
  acceptButton->setObjectName("Accept");
  acceptButton->setText(tr("Accept"));
  QPushButton* resetButton = new QPushButton(this);
  resetButton->setObjectName("Reset");
  resetButton->setText(tr("Reset"));
  buttonlayout->addWidget(acceptButton);
  buttonlayout->addWidget(resetButton);

  this->connect(acceptButton, SIGNAL(pressed()), SLOT(accept()));
  this->connect(resetButton, SIGNAL(pressed()), SLOT(reset()));
}
/// destructor
pqObjectEditor::~pqObjectEditor()
{
}

/// set the proxy to display properties for
void pqObjectEditor::setProxy(pqSMProxy p)
{
  if(this->Proxy)
    {
    this->deleteWidgets();
    }
  this->Proxy = p;
  if(this->Proxy)
    {
    this->createWidgets();
    this->getServerManagerProperties(this->Proxy, this);
    }
}

/// get the proxy for which properties are displayed
pqSMProxy pqObjectEditor::proxy()
{
  return this->Proxy;
}

/// accept the changes made to the properties
/// changes will be propogated down to the server manager
void pqObjectEditor::accept()
{
  this->setServerManagerProperties(this->Proxy, this);
  
  // cause the screen to update
  QVTKWidget *qwindow = pqPipelineData::instance()->getWindowFor(this->Proxy);
  if(qwindow)
    {
      qwindow->update();
    }
}

/// reset the changes made
/// editor will query properties from the server manager
void pqObjectEditor::reset()
{
  this->getServerManagerProperties(this->Proxy, this);
}

void pqObjectEditor::createWidgets()
{

  if(!this->Proxy)
    {
    return;
    }

  int rowCount = 0;

  // query for proxy properties, and create widgets
  vtkSMPropertyIterator *iter = this->Proxy->NewPropertyIterator();
  for(iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
    vtkSMProperty* SMProperty = iter->GetProperty();

    // skip information properties
    if(SMProperty->GetInformationOnly())
      {
      continue;
      }
    pqSMAdaptor::PropertyType pt = pqSMAdaptor::getPropertyType(SMProperty);

    // skip input properties
    if(pt == pqSMAdaptor::PROXY || pt == pqSMAdaptor::PROXYLIST)
      {
      if(SMProperty == this->Proxy->GetProperty("Input"))
        {
        continue;
        }
      }

    if(pt == pqSMAdaptor::PROXY)
      {
      // create a combo box with list of proxies
      QComboBox* combo = new QComboBox(this->PanelLayout->parentWidget());
      combo->setObjectName(iter->GetKey());
      QLabel* label = new QLabel(this->PanelLayout->parentWidget());
      label->setText(iter->GetKey());
      this->PanelLayout->addWidget(label, rowCount, 0, 1, 1);
      this->PanelLayout->addWidget(combo, rowCount, 1, 1, 1);
      rowCount++;
      }
    else if(pt == pqSMAdaptor::PROXYLIST)
      {
      // create a list of selections of proxies
      }
    else if(pt == pqSMAdaptor::ENUMERATION)
      {
      // TODO: filenames show up in combo boxes .. gotta fix that
      QVariant enum_property = pqSMAdaptor::getEnumerationProperty(this->Proxy, 
        SMProperty);
      if(enum_property.type() == QVariant::Bool)
        {
        // check box for true/false
        QCheckBox* check = new QCheckBox(iter->GetKey(), this->PanelLayout->parentWidget());
        check->setObjectName(iter->GetKey());
        this->PanelLayout->addWidget(check, rowCount, 0, 1, 2);
        rowCount++;
        }
      else
        {
        // combo box with strings
        QComboBox* combo = new QComboBox(this->PanelLayout->parentWidget());
        combo->setObjectName(iter->GetKey());
        QLabel* label = new QLabel(this->PanelLayout->parentWidget());
        label->setText(iter->GetKey());

        this->PanelLayout->addWidget(label, rowCount, 0, 1, 1);
        this->PanelLayout->addWidget(combo, rowCount, 1, 1, 1);
        rowCount++;
        }
      }
    else if(pt == pqSMAdaptor::SELECTION)
      {
      QList<QList<QVariant> > items = pqSMAdaptor::getSelectionProperty(this->Proxy, SMProperty);
      QListWidget* lw = new QListWidget(this->PanelLayout->parentWidget());
      lw->setObjectName(iter->GetKey());
      this->PanelLayout->addWidget(lw, rowCount, 0, 1, 2);
      rowCount++;
      }
    else if(pt == pqSMAdaptor::SINGLE_ELEMENT)
      {
      QVariant elem_property = pqSMAdaptor::getElementProperty(this->Proxy, SMProperty);
      QList<QVariant> propertyDomain = pqSMAdaptor::getElementPropertyDomain(SMProperty);
      if(elem_property.type() == QVariant::String && propertyDomain.size())
        {
        // combo box with strings
        QComboBox* combo = new QComboBox(this->PanelLayout->parentWidget());
        combo->setObjectName(iter->GetKey());
        QLabel* label = new QLabel(this->PanelLayout->parentWidget());
        label->setText(iter->GetKey());
        
        this->PanelLayout->addWidget(label, rowCount, 0, 1, 1);
        this->PanelLayout->addWidget(combo, rowCount, 1, 1, 1);
        rowCount++;
        }
      else
        {
        QLineEdit* lineEdit = new QLineEdit(this->PanelLayout->parentWidget());
        lineEdit->setObjectName(iter->GetKey());
        QLabel* label = new QLabel(this->PanelLayout->parentWidget());
        label->setText(iter->GetKey());
        
        this->PanelLayout->addWidget(label, rowCount, 0, 1, 1);
        this->PanelLayout->addWidget(lineEdit, rowCount, 1, 1, 1);
        rowCount++;
        }
      }
    else if(pt == pqSMAdaptor::MULTIPLE_ELEMENTS)
      {
      QList<QVariant> list_property = pqSMAdaptor::getMultipleElementProperty(this->Proxy, SMProperty);
      QLabel* label = new QLabel(this->PanelLayout->parentWidget());
      label->setText(iter->GetKey());
      this->PanelLayout->addWidget(label, rowCount, 0, 1, 1);
      QHBoxLayout* hlayout = new QHBoxLayout;
      hlayout->setObjectName(iter->GetKey());
      this->PanelLayout->addItem(hlayout, rowCount, 1, 1, 1);
      
      int i=0;
      foreach(QVariant v, list_property)
        {
        QLineEdit* lineEdit = new QLineEdit(this->PanelLayout->parentWidget());
        QString num;
        num.setNum(i);
        lineEdit->setObjectName(QString(iter->GetKey()) + QString(":") + num);
        hlayout->addWidget(lineEdit);
        lineEdit->show();
        i++;
        }
        rowCount++;
      }
    else if(pt == pqSMAdaptor::FILE_LIST)
      {
      QLineEdit* lineEdit = new QLineEdit(this->PanelLayout->parentWidget());
      lineEdit->setObjectName(iter->GetKey());
      QLabel* label = new QLabel(this->PanelLayout->parentWidget());
      label->setText(iter->GetKey());
      
      this->PanelLayout->addWidget(label, rowCount, 0, 1, 1);
      this->PanelLayout->addWidget(lineEdit, rowCount, 1, 1, 1);
      rowCount++;
      }
    }
  iter->Delete();
  this->PanelLayout->addItem(new QSpacerItem(0,0,QSizePolicy::Expanding,QSizePolicy::Expanding), rowCount, 0, 1, 2);
}

static void pqObjectEditorDeleteWidgets(QLayoutItem* item)
{
  if(item->widget())
    {
    delete item->widget();
    }
  else if(item->layout())
    {
    QLayoutItem *child;
    while ((child = item->layout()->takeAt(0)) != 0) 
      {
      pqObjectEditorDeleteWidgets(child);
      delete child;
      }
    delete item->layout();
    }
}

void pqObjectEditor::deleteWidgets()
{
  // delete all child widgets
  QLayoutItem *child;
  while ((child = this->PanelLayout->takeAt(0)) != 0) 
    {
    pqObjectEditorDeleteWidgets(child);
    }
}

void pqObjectEditor::getServerManagerProperties(pqSMProxy proxy, QWidget* widget)
{
  vtkSMPropertyIterator *iter = proxy->NewPropertyIterator();
  for(iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
    vtkSMProperty* SMProperty = iter->GetProperty();
    if(SMProperty->GetInformationOnly())
      {
      continue;
      }

    QWidget* foundWidget = widget->findChild<QWidget*>(iter->GetKey());
    pqSMAdaptor::PropertyType pt = pqSMAdaptor::getPropertyType(SMProperty);

    // a layout was found with the name of the property
    // layouts can contain multiple widgets with property values
    if(pt == pqSMAdaptor::MULTIPLE_ELEMENTS)
      {
      QList<QList<QVariant> > domain = pqSMAdaptor::getMultipleElementPropertyDomain(SMProperty);
      for(int i=0; i<domain.size(); i++)
        {
        QString name;
        name.setNum(i);
        name = QString(iter->GetKey()) + QString(":") + name;
        QLineEdit* le = foundWidget->findChild<QLineEdit*>(name);
        if(le)
          {
          QVariant prop = pqSMAdaptor::getMultipleElementProperty(proxy, SMProperty, i);
          le->setText(prop.toString());
          }
        }
      }
    // a layout was found with the name of the property
    else if(foundWidget)
      {
      if(pt == pqSMAdaptor::ENUMERATION)
        {
        // enumerations can be combo boxes or check boxes
        QCheckBox* checkBox = qobject_cast<QCheckBox*>(foundWidget);
        QComboBox* comboBox = qobject_cast<QComboBox*>(foundWidget);
        QLineEdit* lineEdit = qobject_cast<QLineEdit*>(foundWidget);
        if(checkBox)
          {
          QVariant enum_property = pqSMAdaptor::getEnumerationProperty(proxy, SMProperty);
          checkBox->setChecked(enum_property.toBool());
          }
        else if(comboBox)
          {
          QVariant enum_property = pqSMAdaptor::getEnumerationProperty(proxy, SMProperty);
          QList<QVariant> domain = pqSMAdaptor::getEnumerationPropertyDomain(SMProperty);
          comboBox->clear();
          foreach(QVariant v, domain)
            {
            comboBox->addItem(v.toString());
            }
          comboBox->setCurrentIndex(comboBox->findText(enum_property.toString()));
          }
        else if(lineEdit)
          {
          QVariant enum_property = pqSMAdaptor::getEnumerationProperty(proxy, SMProperty);
          lineEdit->setText(enum_property.toString());
          }
        }
      else if(pt == pqSMAdaptor::SELECTION)
        {
        // selections can be list widgets
        QListWidget* listWidget = qobject_cast<QListWidget*>(foundWidget);
        if(listWidget)
          {
          listWidget->clear();
          QList<QList<QVariant> > sel_property = pqSMAdaptor::getSelectionProperty(proxy, SMProperty);; 
          foreach(QList<QVariant> v, sel_property)
            {
            QListWidgetItem* item = new QListWidgetItem(v[0].toString(), listWidget);
            if(v[1].toBool())
              {
              item->setCheckState(Qt::Checked);
              }
            else
              {
              item->setCheckState(Qt::Unchecked);
              }
            }
          }
        }
      else if(pt == pqSMAdaptor::PROXY)
        {
        QComboBox* comboBox = qobject_cast<QComboBox*>(foundWidget);
        if(comboBox)
          {
          QList<pqSMProxy> propertyDomain = pqSMAdaptor::getProxyPropertyDomain(proxy, SMProperty);
          pqSMProxy proxy_property = pqSMAdaptor::getProxyProperty(proxy, SMProperty);
          comboBox->clear();
          int currentIndex=0;
          int i=0;
          foreach(pqSMProxy v, propertyDomain)
            {
            if(proxy_property == v)
              {
              currentIndex = i;
              }
            i++;
            pqPipelineObject* o = pqPipelineData::instance()->getObjectFor(v);
            if(o)
              {
              comboBox->addItem(o->GetProxyName());
              }
            else
              {
              comboBox->addItem("No Name");
              }
            }
          comboBox->setCurrentIndex(currentIndex);
          }
        }
      else if(pt == pqSMAdaptor::SINGLE_ELEMENT)
        {
        QComboBox* comboBox = qobject_cast<QComboBox*>(foundWidget);
        QLineEdit* lineEdit = qobject_cast<QLineEdit*>(foundWidget);
        if(comboBox)
          {
          QVariant elem_property = pqSMAdaptor::getElementProperty(proxy, SMProperty);
          QList<QVariant> domain = pqSMAdaptor::getElementPropertyDomain(SMProperty);
          comboBox->clear();
          int currentIndex=0;
          int i=0;
          foreach(QVariant v, domain)
            {
            if(elem_property == v)
              {
              currentIndex = i;
              }
            i++;
            comboBox->addItem(v.toString());
            }
          comboBox->setCurrentIndex(currentIndex);
          }
        else if(lineEdit)
          {
          lineEdit->setText(pqSMAdaptor::getElementProperty(proxy, SMProperty).toString());
          }
        }
      else if(pt == pqSMAdaptor::FILE_LIST)
        {
        QLineEdit* lineEdit = qobject_cast<QLineEdit*>(foundWidget);
        if(lineEdit)
          {
          lineEdit->setText(pqSMAdaptor::getFileListProperty(proxy, SMProperty));
          }
        }
      }
    }
  iter->Delete();

}

void pqObjectEditor::setServerManagerProperties(pqSMProxy proxy, QWidget* widget)
{
  vtkSMPropertyIterator *iter = proxy->NewPropertyIterator();
  for(iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
    vtkSMProperty* SMProperty = iter->GetProperty();
    if(SMProperty->GetInformationOnly())
      {
      continue;
      }

    QWidget* foundWidget = widget->findChild<QWidget*>(iter->GetKey());
    pqSMAdaptor::PropertyType pt = pqSMAdaptor::getPropertyType(SMProperty);

    // a layout was found with the name of the property
    // layouts can contain multiple widgets with property values
    if(pt == pqSMAdaptor::MULTIPLE_ELEMENTS)
      {
      QList<QList<QVariant> > domain = pqSMAdaptor::getMultipleElementPropertyDomain(SMProperty);
      for(int i=0; i<domain.size(); i++)
        {
        QString name;
        name.setNum(i);
        name = QString(iter->GetKey()) + QString(":") + name;
        QLineEdit* le = foundWidget->findChild<QLineEdit*>(name);
        if(le)
          {
          pqSMAdaptor::setMultipleElementProperty(proxy, SMProperty, i, le->text());
          }
        }
      }
    // a layout was found with the name of the property
    else if(foundWidget)
      {
      if(pt == pqSMAdaptor::ENUMERATION)
        {
        // enumerations can be combo boxes or check boxes
        QCheckBox* checkBox = qobject_cast<QCheckBox*>(foundWidget);
        QComboBox* comboBox = qobject_cast<QComboBox*>(foundWidget);
        QLineEdit* lineEdit = qobject_cast<QLineEdit*>(foundWidget);
        if(checkBox)
          {
          pqSMAdaptor::setEnumerationProperty(proxy, SMProperty, checkBox->isChecked());
          }
        else if(comboBox && comboBox->count())
          {
          pqSMAdaptor::setEnumerationProperty(proxy, SMProperty, comboBox->currentText());
          }
        else if(lineEdit)
          {
          pqSMAdaptor::setEnumerationProperty(proxy, SMProperty, lineEdit->text());
          }
        }
      else if(pt == pqSMAdaptor::SELECTION)
        {
        // selections can be list widgets
        QListWidget* listWidget = qobject_cast<QListWidget*>(foundWidget);
        if(listWidget)
          {
          QList<QList<QVariant> > list_property; 
          for(int i=0; i<listWidget->count(); i++)
            {
            QListWidgetItem* item = listWidget->item(i);
            QList<QVariant> prop;
            prop.append(item->text());
            prop.append(item->checkState() == Qt::Checked);
            list_property.append(prop);
            }
          pqSMAdaptor::setSelectionProperty(proxy, SMProperty, list_property);
          }
        }
      else if(pt == pqSMAdaptor::PROXY)
        {
        QComboBox* comboBox = qobject_cast<QComboBox*>(foundWidget);
        if(comboBox && comboBox->count())
          {
          QList<pqSMProxy> propertyDomain = pqSMAdaptor::getProxyPropertyDomain(proxy, SMProperty);
          pqSMAdaptor::setProxyProperty(proxy, SMProperty, propertyDomain[comboBox->currentIndex()]);
          }
        }
      else if(pt == pqSMAdaptor::SINGLE_ELEMENT)
        {
        QComboBox* comboBox = qobject_cast<QComboBox*>(foundWidget);
        QLineEdit* lineEdit = qobject_cast<QLineEdit*>(foundWidget);
        if(comboBox && comboBox->count())
          {
          pqSMAdaptor::setElementProperty(proxy, SMProperty, comboBox->currentText());
          }
        else if(lineEdit)
          {
          pqSMAdaptor::setElementProperty(proxy, SMProperty, lineEdit->text());
          }
        }
      else if(pt == pqSMAdaptor::FILE_LIST)
        {
        QLineEdit* lineEdit = qobject_cast<QLineEdit*>(foundWidget);
        if(lineEdit)
          {
          pqSMAdaptor::setFileListProperty(proxy, SMProperty, lineEdit->text());
          }
        }
      }
    }
  iter->Delete();
  proxy->UpdateVTKObjects();
}


