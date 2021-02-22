/*=========================================================================

   Program: ParaView
   Module:    pqCustomFilterDefinitionWizard.cxx

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

========================================================================*/

/// \file pqCustomFilterDefinitionWizard.cxx
/// \date 6/19/2006

#include "pqCustomFilterDefinitionWizard.h"
#include "ui_pqCustomFilterDefinitionWizard.h"

#include "pqCustomFilterDefinitionModel.h"
#include "pqCustomFilterManagerModel.h"
#include "pqFlatTreeView.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"

#include <QHeaderView>
#include <QMessageBox>
#include <QModelIndex>
#include <QStringList>
#include <QTreeWidget>
#include <QTreeWidgetItem>

#include "vtkPVXMLElement.h"
#include "vtkSMCompoundSourceProxy.h"
#include "vtkSMInputProperty.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSmartPointer.h"

class pqCustomFilterDefinitionWizardForm : public Ui::pqCustomFilterDefinitionWizard
{
public:
  QStringList ExposedPropertyNames; ///< Used to ensure all exposed
                                    ///  property names are unique.

  QStringList ExposedPortNames; ///< Used to ensure all exposed
                                ///  port names are unique.
  QStringList ToExposeNames;    ///< Used to ensure that same thing
                                /// (either property or port) is not exposed twice.

  // Maps a proxy name with a map of <propertyLabel, propertyName>
  // pairs for each of its properties. This assumes that labels of
  // a proxy's properties are unique.
  QMap<QString, QMap<QString, QString> > LabelToNamePropertyMap;
};

//-----------------------------------------------------------------------------
pqCustomFilterDefinitionWizard::pqCustomFilterDefinitionWizard(
  pqCustomFilterDefinitionModel* model, QWidget* widgetParent)
  : QDialog(widgetParent)
{
  this->CurrentPage = 0;
  this->OverwriteOK = false;
  this->Filter = nullptr;
  this->Model = model;
  this->Form = new pqCustomFilterDefinitionWizardForm();
  this->Form->setupUi(this);

  // Initialize the form.
  this->Form->TitleFrame->setBackgroundRole(QPalette::Base);
  this->Form->TitleFrame->setAutoFillBackground(true);
  this->Form->TitleStack->setCurrentIndex(this->CurrentPage);
  this->Form->PageStack->setCurrentIndex(this->CurrentPage);
  this->Form->BackButton->setEnabled(false);
  this->Form->RemoveInputButton->setEnabled(false);
  this->Form->InputUpButton->setEnabled(false);
  this->Form->InputDownButton->setEnabled(false);
  this->Form->RemoveOutputButton->setEnabled(false);
  this->Form->OutputUpButton->setEnabled(false);
  this->Form->OutputDownButton->setEnabled(false);
  this->Form->RemovePropertyButton->setEnabled(false);
  this->Form->PropertyUpButton->setEnabled(false);
  this->Form->PropertyDownButton->setEnabled(false);

  this->Form->InputPipeline->getHeader()->hide();
  this->Form->OutputPipeline->getHeader()->hide();
  this->Form->PropertyPipeline->getHeader()->hide();
  this->Form->InputPipeline->setModel(this->Model);
  this->Form->OutputPipeline->setModel(this->Model);
  this->Form->PropertyPipeline->setModel(this->Model);

  this->setupDefaultInputOutput();

  // Listen to the button click events.
  QObject::connect(this->Form->CancelButton, SIGNAL(clicked()), this, SLOT(reject()));
  QObject::connect(this->Form->BackButton, SIGNAL(clicked()), this, SLOT(navigateBack()));
  QObject::connect(this->Form->NextButton, SIGNAL(clicked()), this, SLOT(navigateNext()));
  QObject::connect(this->Form->FinishButton, SIGNAL(clicked()), this, SLOT(finishWizard()));

  // Listen to the selection events.
  QObject::connect(this->Form->InputPipeline->getSelectionModel(),
    SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)), this,
    SLOT(updateInputForm(const QModelIndex&, const QModelIndex&)));
  QObject::connect(this->Form->OutputPipeline->getSelectionModel(),
    SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)), this,
    SLOT(updateOutputForm(const QModelIndex&, const QModelIndex&)));
  QObject::connect(this->Form->PropertyPipeline->getSelectionModel(),
    SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)), this,
    SLOT(updatePropertyForm(const QModelIndex&, const QModelIndex&)));
  QObject::connect(this->Form->InputPorts->selectionModel(),
    SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)), this,
    SLOT(updateInputButtons(const QModelIndex&, const QModelIndex&)));
  QObject::connect(this->Form->OutputPorts->selectionModel(),
    SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)), this,
    SLOT(updateOutputButtons(const QModelIndex&, const QModelIndex&)));
  QObject::connect(this->Form->PropertyList->selectionModel(),
    SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)), this,
    SLOT(updatePropertyButtons(const QModelIndex&, const QModelIndex&)));

  // Listen to the list editing buttons.
  QObject::connect(this->Form->AddInputButton, SIGNAL(clicked()), this, SLOT(addInput()));
  QObject::connect(this->Form->RemoveInputButton, SIGNAL(clicked()), this, SLOT(removeInput()));
  QObject::connect(this->Form->InputUpButton, SIGNAL(clicked()), this, SLOT(moveInputUp()));
  QObject::connect(this->Form->InputDownButton, SIGNAL(clicked()), this, SLOT(moveInputDown()));
  QObject::connect(this->Form->AddOutputButton, SIGNAL(clicked()), this, SLOT(addOutput()));
  QObject::connect(this->Form->RemoveOutputButton, SIGNAL(clicked()), this, SLOT(removeOutput()));
  QObject::connect(this->Form->OutputUpButton, SIGNAL(clicked()), this, SLOT(moveOutputUp()));
  QObject::connect(this->Form->OutputDownButton, SIGNAL(clicked()), this, SLOT(moveOutputDown()));
  QObject::connect(this->Form->AddPropertyButton, SIGNAL(clicked()), this, SLOT(addProperty()));
  QObject::connect(
    this->Form->RemovePropertyButton, SIGNAL(clicked()), this, SLOT(removeProperty()));
  QObject::connect(this->Form->PropertyUpButton, SIGNAL(clicked()), this, SLOT(movePropertyUp()));
  QObject::connect(
    this->Form->PropertyDownButton, SIGNAL(clicked()), this, SLOT(movePropertyDown()));

  // Listen for name changes.
  QObject::connect(this->Form->CustomFilterName, SIGNAL(textEdited(const QString&)), this,
    SLOT(clearNameOverwrite(const QString&)));

  // When combo selection changes, update new label to be same as old.
  QObject::connect(this->Form->PropertyCombo, SIGNAL(currentIndexChanged(const QString&)),
    this->Form->PropertyName, SLOT(setText(const QString&)));
}

//-----------------------------------------------------------------------------
pqCustomFilterDefinitionWizard::~pqCustomFilterDefinitionWizard()
{
  if (this->Form)
  {
    delete this->Form;
  }

  if (this->Filter)
  {
    // Release the reference to the compound proxy.
    this->Filter->Delete();
  }
}

//-----------------------------------------------------------------------------
QString pqCustomFilterDefinitionWizard::getCustomFilterName() const
{
  if (this->Filter)
  {
    return this->Form->CustomFilterName->text();
  }

  return QString();
}

//-----------------------------------------------------------------------------
void pqCustomFilterDefinitionWizard::createCustomFilter()
{
  if (this->Filter != nullptr || this->Form->CustomFilterName->text().isEmpty())
  {
    return;
  }

  // Create the compound proxy. Add all the proxies to it.
  pqPipelineSource* source = nullptr;
  this->Filter = vtkSMCompoundSourceProxy::New();
  bool first = true;
  QModelIndex index = this->Model->getNextIndex(QModelIndex());
  while (index.isValid())
  {
    source = this->Model->getSourceFor(index);
    if (source)
    {
      if (first)
      {
        this->Filter->SetSession(source->getProxy()->GetSession());
        this->Filter->SetLocation(source->getProxy()->GetLocation());
        first = false;
      }
      this->Filter->AddProxy(source->getSMName().toLocal8Bit().data(), source->getProxy());
    }

    index = this->Model->getNextIndex(index);
  }

  // Expose the input properties.
  int i = 0;
  QTreeWidgetItem* item = nullptr;
  int numInputs = this->Form->InputPorts->topLevelItemCount();
  for (; i < numInputs; i++)
  {
    item = this->Form->InputPorts->topLevelItem(i);
    this->Filter->ExposeProperty(item->text(0).toLocal8Bit().data(),
      this->Form->LabelToNamePropertyMap[item->text(0)][item->text(1)].toLocal8Bit().data(),
      item->text(2).toLocal8Bit().data());
  }

  // Set the output proxies.
  int total = this->Form->OutputPorts->topLevelItemCount();
  for (i = 0; i < total; i++)
  {
    item = this->Form->OutputPorts->topLevelItem(i);
    pqOutputPort* port = qobject_cast<pqOutputPort*>(item->data(0, Qt::UserRole).value<QObject*>());
    if (port)
    {
      this->Filter->ExposeOutputPort(port->getSource()->getSMName().toLocal8Bit().data(),
        port->getPortNumber(), item->text(1).toLocal8Bit().data());
    }
  }

  // Expose the properties.
  total = this->Form->PropertyList->topLevelItemCount();
  for (i = 0; i < total; i++)
  {
    item = this->Form->PropertyList->topLevelItem(i);
    this->Filter->ExposeProperty(item->text(0).toLocal8Bit().data(),
      this->Form->LabelToNamePropertyMap[item->text(0)][item->text(1)].toLocal8Bit().data(),
      item->text(2).toLocal8Bit().data());
  }

  // Include any internal proxies.
  this->addAutoIncludedProxies();

  // Register the compound proxy definition with the server manager.
  vtkPVXMLElement* root = this->Filter->SaveDefinition(nullptr);
  vtkSMSessionProxyManager* proxyManager = source->proxyManager();
  if (numInputs > 0)
  {
    proxyManager->RegisterCustomProxyDefinition(
      "filters", this->Form->CustomFilterName->text().toLocal8Bit().data(), root);
  }
  else
  {
    proxyManager->RegisterCustomProxyDefinition(
      "sources", this->Form->CustomFilterName->text().toLocal8Bit().data(), root);
  }
  root->Delete();
}

//-----------------------------------------------------------------------------
void pqCustomFilterDefinitionWizard::addAutoIncludedProxies()
{
  unsigned int num_of_proxies = this->Filter->GetNumberOfProxies();
  vtkSMSessionProxyManager* pxm =
    vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();

  QSet<vtkSMProxy*> autoIncludeSet;

  for (unsigned int cc = 0; cc < num_of_proxies; cc++)
  {
    vtkSMProxy* subProxy = this->Filter->GetProxy(cc);
    vtkSmartPointer<vtkSMPropertyIterator> iter;
    iter.TakeReference(subProxy->NewPropertyIterator());

    for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
      vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(iter->GetProperty());
      if (!pp)
      {
        continue;
      }
      unsigned int proxy_count = pp->GetNumberOfProxies();
      for (unsigned int i = 0; i < proxy_count; i++)
      {
        vtkSMProxy* proxy = pp->GetProxy(i);
        if (!proxy || pxm->GetProxyName("sources", proxy) != nullptr)
        {
          continue;
        }
        autoIncludeSet.insert(proxy);
      }
    }
  }
  foreach (vtkSMProxy* proxy, autoIncludeSet)
  {
    QString name = "auto_";
    name += proxy->GetGlobalIDAsString();
    this->Filter->AddProxy(name.toLocal8Bit().data(), proxy);
  }
}

//-----------------------------------------------------------------------------
bool pqCustomFilterDefinitionWizard::validateCustomFilterName()
{
  // Make sure the user has entered a name for the custom filter.
  QString filterName = this->Form->CustomFilterName->text();
  if (filterName.isEmpty())
  {
    QMessageBox::warning(this, "No Name", "The custom filter name field is empty.\n"
                                          "Please enter a unique name for the custom filter.",
      QMessageBox::Ok | QMessageBox::Default, QMessageBox::NoButton);
    this->Form->CustomFilterName->setFocus();
    return false;
  }

  // Make sure the name is unique.
  vtkSMSessionProxyManager* proxyManager =
    vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();
  if (!this->OverwriteOK)
  {
    if (proxyManager->GetProxyDefinition("filters", filterName.toLocal8Bit().data()) ||
      proxyManager->GetProxyDefinition("sources", filterName.toLocal8Bit().data()))
    {
      QMessageBox::warning(this, "Duplicate Name", "This filter name already exists.\n"
                                                   "Please enter a different name.",
        QMessageBox::Ok | QMessageBox::Default, QMessageBox::NoButton);
      return false;
    }
  }

  return true;
}

//-----------------------------------------------------------------------------
void pqCustomFilterDefinitionWizard::setupDefaultInputOutput()
{
  if (this->Model->rowCount() == 1)
  {
    // Set up the default input property.
    QModelIndex index = this->Model->index(0, 0);
    pqPipelineSource* source = this->Model->getSourceFor(index);
    if (source)
    {
      vtkSMProxy* proxy = source->getProxy();
      if (proxy)
      {
        QStringList inputNames;
        vtkSMInputProperty* input = nullptr;
        vtkSMPropertyIterator* iter = proxy->NewPropertyIterator();
        for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
        {
          input = vtkSMInputProperty::SafeDownCast(iter->GetProperty());
          if (input)
          {
            inputNames.append(QString(iter->GetKey()));
          }
        }

        iter->Delete();
        if (inputNames.size() > 0)
        {
          // Add the "Input" property if it exists. Otherwise, use the
          // first property.
          QString propertyName = "Input";
          if (!inputNames.contains("Input"))
          {
            propertyName = inputNames[0];
          }

          QStringList list;
          list.append(source->getSMName());
          list.append(propertyName);
          list.append("Input");
          QTreeWidgetItem* item = new QTreeWidgetItem(this->Form->InputPorts, list);
          this->Form->LabelToNamePropertyMap[source->getSMName()][propertyName] = propertyName;
          this->Form->InputPorts->setCurrentItem(item);
          this->Form->ExposedPropertyNames.append("Input");
          this->Form->ToExposeNames.append(
            QString("INPUT:%1.%2").arg(item->text(0)).arg(item->text(1)));
        }
      }
    }

    // Set up the default output property.
    while (this->Model->hasChildren(index))
    {
      if (this->Model->rowCount(index) > 0)
      {
        index = this->Model->index(0, 0, index);
      }
      else
      {
        index = QModelIndex();
        break;
      }
    }

    pqPipelineSource* output = this->Model->getSourceFor(index);
    if (output)
    {
      this->addOutputInternal(output->getOutputPort(0), "Output");
    }
  }
}

//-----------------------------------------------------------------------------
void pqCustomFilterDefinitionWizard::navigateBack()
{
  if (this->CurrentPage > 0)
  {
    // Decrement the page number and set the stacks to the appropriate
    // page. Update the button state as needed.
    this->CurrentPage--;
    this->Form->TitleStack->setCurrentIndex(this->CurrentPage);
    this->Form->PageStack->setCurrentIndex(this->CurrentPage);
    if (this->CurrentPage == 0)
    {
      this->Form->BackButton->setEnabled(false);
    }
    else if (this->CurrentPage == 2)
    {
      this->Form->NextButton->setEnabled(true);
    }
  }
}

//-----------------------------------------------------------------------------
void pqCustomFilterDefinitionWizard::navigateNext()
{
  if (this->CurrentPage < 3)
  {
    if (this->CurrentPage == 0 && !this->validateCustomFilterName())
    {
      return;
    }

    // Increment the page number and set the stacks to the appropriate
    // page. Update the button state as needed.
    this->CurrentPage++;
    this->Form->TitleStack->setCurrentIndex(this->CurrentPage);
    this->Form->PageStack->setCurrentIndex(this->CurrentPage);
    if (this->CurrentPage == 1)
    {
      this->Form->BackButton->setEnabled(true);
    }
    else if (this->CurrentPage == 3)
    {
      this->Form->NextButton->setEnabled(false);
    }
  }
}

//-----------------------------------------------------------------------------
void pqCustomFilterDefinitionWizard::finishWizard()
{
  // Make sure the name has been entered and is unique.
  if (this->validateCustomFilterName())
  {
    this->accept();
  }
}

//-----------------------------------------------------------------------------
void pqCustomFilterDefinitionWizard::clearNameOverwrite(const QString&)
{
  this->OverwriteOK = false;
}

//-----------------------------------------------------------------------------
void pqCustomFilterDefinitionWizard::updateInputForm(const QModelIndex& current, const QModelIndex&)
{
  // Clear the form fields.
  this->Form->InputName->setText("");
  this->Form->InputCombo->clear();

  // Get the proxy from the index. Use the proxy to find all the input
  // properties to put in the combo box.
  pqPipelineSource* source = this->Model->getSourceFor(current);
  if (source)
  {
    vtkSMProxy* proxy = source->getProxy();
    if (proxy)
    {
      vtkSMInputProperty* input = nullptr;
      vtkSMPropertyIterator* iter = proxy->NewPropertyIterator();
      for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
      {
        input = vtkSMInputProperty::SafeDownCast(iter->GetProperty());
        if (input)
        {
          const char* name = iter->GetProperty()->GetXMLLabel();
          if (!name)
          {
            name = iter->GetKey();
          }
          this->Form->LabelToNamePropertyMap[source->getSMName()][name] = iter->GetKey();
          this->Form->InputCombo->addItem(name);
        }
      }

      iter->Delete();
    }
  }
}

//-----------------------------------------------------------------------------
void pqCustomFilterDefinitionWizard::updateOutputForm(
  const QModelIndex& current, const QModelIndex&)
{
  // Clear the output name field.
  this->Form->OutputName->setText("");
  this->Form->OutputCombo->clear();

  // * Get the proxy from the index.
  // * Find the names for all output ports for that proxy.
  // * Populate the OutputCombo with available options.
  pqPipelineSource* source = this->Model->getSourceFor(current);
  if (source)
  {
    vtkSMSourceProxy* proxy = vtkSMSourceProxy::SafeDownCast(source->getProxy());
    if (proxy)
    {
      unsigned int numPorts = proxy->GetNumberOfOutputPorts();
      for (unsigned int cc = 0; cc < numPorts; cc++)
      {
        this->Form->OutputCombo->addItem(proxy->GetOutputPortName(cc));
      }
    }
  }
}

//-----------------------------------------------------------------------------
void pqCustomFilterDefinitionWizard::updatePropertyForm(
  const QModelIndex& current, const QModelIndex&)
{
  // Clear the form fields.
  this->Form->PropertyName->setText("");
  this->Form->PropertyCombo->clear();
  // this->Form->LabelToNamePropertyMap.clear();

  // Get the proxy from the index. Use the proxy to find all the
  // properties to put in the combo box. Don't display the input
  // properties.
  pqPipelineSource* source = this->Model->getSourceFor(current);
  if (source)
  {
    vtkSMProxy* proxy = source->getProxy();
    if (proxy)
    {
      vtkSMInputProperty* input = nullptr;
      vtkSMPropertyIterator* iter = proxy->NewPropertyIterator();
      for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
      {
        input = vtkSMInputProperty::SafeDownCast(iter->GetProperty());
        if (!input)
        {
          const char* name = iter->GetProperty()->GetXMLLabel();
          if (!name)
          {
            name = iter->GetKey();
          }
          this->Form->LabelToNamePropertyMap[source->getSMName()][name] = iter->GetKey();
          this->Form->PropertyCombo->addItem(name);
        }
      }

      iter->Delete();
    }
  }
}

//-----------------------------------------------------------------------------
void pqCustomFilterDefinitionWizard::addInput()
{
  // Validate the entry. Make sure there is an object and a property
  // selected. Make sure the name is unique.
  pqPipelineSource* source =
    this->Model->getSourceFor(this->Form->InputPipeline->getSelectionModel()->currentIndex());
  if (!source)
  {
    QMessageBox::warning(this, "No Object Selected",
      "No pipeline object is selected.\n"
      "Please select a pipeline object from the list on the left.",
      QMessageBox::Ok | QMessageBox::Default, QMessageBox::NoButton);
    return;
  }

  int propertyIndex = this->Form->InputCombo->currentIndex();
  if (propertyIndex == -1)
  {
    QMessageBox::warning(this, "No Input Properties",
      "The selected pipeline object does not have any inputs.\n"
      "Please select another pipeline object from the list on the left.",
      QMessageBox::Ok | QMessageBox::Default, QMessageBox::NoButton);
    return;
  }

  QString name = this->Form->InputName->text();
  if (name.isEmpty())
  {
    QMessageBox::warning(this, "No Name", "The input name field is empty.\n"
                                          "Please enter a unique name for the input.",
      QMessageBox::Ok | QMessageBox::Default, QMessageBox::NoButton);
    this->Form->InputName->setFocus();
    return;
  }

  if (this->Form->ExposedPropertyNames.contains(name))
  {
    QMessageBox::warning(this, "Duplicate Name", "Another input already has the name entered.\n"
                                                 "Please enter a unique name for the input.",
      QMessageBox::Ok | QMessageBox::Default, QMessageBox::NoButton);
    this->Form->InputName->setFocus();
    this->Form->InputName->selectAll();
    return;
  }

  QString key = QString("INPUT:%1.%2")
                  .arg(source->getSMName())
                  .arg(this->Form->InputCombo->itemText(propertyIndex));
  if (this->Form->ToExposeNames.contains(key))
  {
    QMessageBox::warning(this, "Duplicate Input", "The selected Input has already been added.",
      QMessageBox::Ok | QMessageBox::Default, QMessageBox::NoButton);
    return;
  }

  // Add the exposed input port to the list.
  QStringList list;
  list.append(source->getSMName());
  list.append(this->Form->InputCombo->itemText(propertyIndex));
  list.append(name);
  QTreeWidgetItem* item = new QTreeWidgetItem(this->Form->InputPorts, list);
  this->Form->InputPorts->setCurrentItem(item);
  this->Form->ExposedPropertyNames.append(name);
  this->Form->ToExposeNames.append(key);
}

//-----------------------------------------------------------------------------
void pqCustomFilterDefinitionWizard::removeInput()
{
  // Remove the selected row from the list.
  QTreeWidgetItem* item = this->Form->InputPorts->currentItem();
  if (item)
  {
    int row = this->Form->InputPorts->indexOfTopLevelItem(item) - 1;
    this->Form->ExposedPropertyNames.removeAll(item->text(2));
    QString key = QString("INPUT:%1.%2").arg(item->text(0)).arg(item->text(1));
    this->Form->ToExposeNames.removeAll(key);
    delete item;
    if (row < 0)
    {
      row = 0;
    }

    item = this->Form->InputPorts->topLevelItem(row);
    if (item)
    {
      this->Form->InputPorts->setCurrentItem(item);
    }
    else
    {
      this->updateInputButtons(QModelIndex(), QModelIndex());
    }
  }
}

//-----------------------------------------------------------------------------
void pqCustomFilterDefinitionWizard::moveInputUp()
{
  // Move the selected row up one if possible.
  QTreeWidgetItem* item = this->Form->InputPorts->currentItem();
  if (item)
  {
    int row = this->Form->InputPorts->indexOfTopLevelItem(item);
    if (row > 0)
    {
      this->Form->InputPorts->takeTopLevelItem(row--);
      this->Form->InputPorts->insertTopLevelItem(row, item);
      this->Form->InputPorts->setCurrentItem(item);
    }
  }
}

//-----------------------------------------------------------------------------
void pqCustomFilterDefinitionWizard::moveInputDown()
{
  // Move the selected row down one if possible.
  QTreeWidgetItem* item = this->Form->InputPorts->currentItem();
  if (item)
  {
    int row = this->Form->InputPorts->indexOfTopLevelItem(item);
    if (row < this->Form->InputPorts->topLevelItemCount() - 1)
    {
      this->Form->InputPorts->takeTopLevelItem(row++);
      this->Form->InputPorts->insertTopLevelItem(row, item);
      this->Form->InputPorts->setCurrentItem(item);
    }
  }
}

//-----------------------------------------------------------------------------
void pqCustomFilterDefinitionWizard::addOutput()
{
  // Validate the entry. Make sure there is an object and a property
  // selected. Make sure the name is unique.
  pqPipelineSource* source =
    this->Model->getSourceFor(this->Form->OutputPipeline->getSelectionModel()->currentIndex());
  if (!source)
  {
    QMessageBox::warning(this, "No Object Selected",
      "No pipeline object is selected.\n"
      "Please select a pipeline object from the list on the left.",
      QMessageBox::Ok | QMessageBox::Default, QMessageBox::NoButton);
    return;
  }

  QString name = this->Form->OutputName->text();
  if (name.isEmpty())
  {
    QMessageBox::warning(this, "No Name", "The output name field is empty.\n"
                                          "Please enter a unique name for the output.",
      QMessageBox::Ok | QMessageBox::Default, QMessageBox::NoButton);
    this->Form->OutputName->setFocus();
    return;
  }

  if (this->Form->ExposedPortNames.contains(name))
  {
    QMessageBox::warning(this, "Duplicate Name", "Another output already has the name entered.\n"
                                                 "Please enter a unique name for the output.",
      QMessageBox::Ok | QMessageBox::Default, QMessageBox::NoButton);
    this->Form->OutputName->setFocus();
    this->Form->OutputName->selectAll();
    return;
  }

  // Validate port name.
  QString portname = this->Form->OutputCombo->currentText();
  pqOutputPort* port = source->getOutputPort(portname);
  if (!port)
  {
    QMessageBox::warning(this, "No Output Port Selected",
      "No output port was selected or selected output port does not exist.\n"
      "Please select a output port from the \"Output Port\" combo box.",
      QMessageBox::Ok | QMessageBox::Default, QMessageBox::NoButton);
    this->Form->OutputCombo->setFocus();
    return;
  }

  // Verify that the selected port hasn't already been exposed.
  // We don't want to export the same output port twice.
  QString key = QString("OUTPUT:%1 (%2)").arg(source->getSMName()).arg(port->getPortNumber());
  if (this->Form->ToExposeNames.contains(key))
  {
    QMessageBox::warning(this, "Duplicate Output", "Selected output port has already been exposed.",
      QMessageBox::Ok | QMessageBox::Default, QMessageBox::NoButton);
    this->Form->OutputCombo->setFocus();
    return;
  }

  this->addOutputInternal(port, name);
}

//-----------------------------------------------------------------------------
void pqCustomFilterDefinitionWizard::addOutputInternal(pqOutputPort* port, const QString& name)
{
  pqPipelineSource* source = port->getSource();
  QString key = QString("OUTPUT:%1 (%2)").arg(source->getSMName()).arg(port->getPortNumber());
  // Add the exposed output port to the list.
  QStringList list;
  if (source->getNumberOfOutputPorts() > 1)
  {
    list.append(QString("%1 (%2)").arg(source->getSMName()).arg(port->getPortNumber()));
  }
  else
  {
    list.append(source->getSMName());
  }
  list.append(name);
  QTreeWidgetItem* item = new QTreeWidgetItem(this->Form->OutputPorts, list);
  item->setData(0, Qt::UserRole, QVariant::fromValue<QObject*>(port));
  this->Form->OutputPorts->setCurrentItem(item);
  this->Form->ExposedPortNames.append(name);
  this->Form->ToExposeNames.append(key);
}

//-----------------------------------------------------------------------------
void pqCustomFilterDefinitionWizard::removeOutput()
{
  // Remove the selected row from the list.
  QTreeWidgetItem* item = this->Form->OutputPorts->currentItem();
  if (item)
  {
    int row = this->Form->OutputPorts->indexOfTopLevelItem(item) - 1;
    this->Form->ExposedPortNames.removeAll(item->text(1));
    pqOutputPort* port = qobject_cast<pqOutputPort*>(item->data(0, Qt::UserRole).value<QObject*>());
    pqPipelineSource* source = port->getSource();
    QString key = QString("OUTPUT:%1 (%2)").arg(source->getSMName()).arg(port->getPortNumber());
    this->Form->ToExposeNames.removeAll(key);
    delete item;
    if (row < 0)
    {
      row = 0;
    }

    item = this->Form->OutputPorts->topLevelItem(row);
    if (item)
    {
      this->Form->OutputPorts->setCurrentItem(item);
    }
    else
    {
      this->updateOutputButtons(QModelIndex(), QModelIndex());
    }
  }
}

//-----------------------------------------------------------------------------
void pqCustomFilterDefinitionWizard::moveOutputUp()
{
  // Move the selected row up one if possible.
  QTreeWidgetItem* item = this->Form->OutputPorts->currentItem();
  if (item)
  {
    int row = this->Form->OutputPorts->indexOfTopLevelItem(item);
    if (row > 0)
    {
      this->Form->OutputPorts->takeTopLevelItem(row--);
      this->Form->OutputPorts->insertTopLevelItem(row, item);
      this->Form->OutputPorts->setCurrentItem(item);
    }
  }
}

//-----------------------------------------------------------------------------
void pqCustomFilterDefinitionWizard::moveOutputDown()
{
  // Move the selected row down one if possible.
  QTreeWidgetItem* item = this->Form->OutputPorts->currentItem();
  if (item)
  {
    int row = this->Form->OutputPorts->indexOfTopLevelItem(item);
    if (row < this->Form->OutputPorts->topLevelItemCount() - 1)
    {
      this->Form->OutputPorts->takeTopLevelItem(row++);
      this->Form->OutputPorts->insertTopLevelItem(row, item);
      this->Form->OutputPorts->setCurrentItem(item);
    }
  }
}

//-----------------------------------------------------------------------------
void pqCustomFilterDefinitionWizard::addProperty()
{
  // Validate the entry. Make sure there is an object and a property
  // selected. Make sure the name is unique.
  pqPipelineSource* source =
    this->Model->getSourceFor(this->Form->PropertyPipeline->getSelectionModel()->currentIndex());
  if (!source)
  {
    QMessageBox::warning(this, "No Object Selected",
      "No pipeline object is selected.\n"
      "Please select a pipeline object from the list on the left.",
      QMessageBox::Ok | QMessageBox::Default, QMessageBox::NoButton);
    return;
  }

  int propertyIndex = this->Form->PropertyCombo->currentIndex();
  if (propertyIndex == -1)
  {
    QMessageBox::warning(this, "No Properties",
      "The selected pipeline object does not have any properties.\n"
      "Please select another pipeline object from the list on the left.",
      QMessageBox::Ok | QMessageBox::Default, QMessageBox::NoButton);
    return;
  }

  QString name = this->Form->PropertyName->text();
  if (name.isEmpty())
  {
    QMessageBox::warning(this, "No Name", "The property name field is empty.\n"
                                          "Please enter a unique name for the property.",
      QMessageBox::Ok | QMessageBox::Default, QMessageBox::NoButton);
    this->Form->PropertyName->setFocus();
    return;
  }

  if (this->Form->ExposedPropertyNames.contains(name))
  {
    QMessageBox::warning(this, "Duplicate Name", "Another property already has the name entered.\n"
                                                 "Please enter a unique name for the property.",
      QMessageBox::Ok | QMessageBox::Default, QMessageBox::NoButton);
    this->Form->PropertyName->setFocus();
    this->Form->PropertyName->selectAll();
    return;
  }

  QString key = QString("INPUT:%1.%2")
                  .arg(source->getSMName())
                  .arg(this->Form->PropertyCombo->itemText(propertyIndex));
  if (this->Form->ToExposeNames.contains(key))
  {
    QMessageBox::warning(this, "Duplicate Property",
      "The selected property have already been exposed.\n"
      "Please select another property.",
      QMessageBox::Ok | QMessageBox::Default, QMessageBox::NoButton);
    this->Form->PropertyCombo->setFocus();
    return;
  }

  // Add the exposed property to the list.
  QStringList list;
  list.append(source->getSMName());
  list.append(this->Form->PropertyCombo->itemText(propertyIndex));
  list.append(name);
  QTreeWidgetItem* item = new QTreeWidgetItem(this->Form->PropertyList, list);
  this->Form->PropertyList->setCurrentItem(item);
  this->Form->ExposedPropertyNames.append(name);
  this->Form->ToExposeNames.append(key);
}

//-----------------------------------------------------------------------------
void pqCustomFilterDefinitionWizard::removeProperty()
{
  // Remove the selected row from the list.
  QTreeWidgetItem* item = this->Form->PropertyList->currentItem();
  if (item)
  {
    int row = this->Form->PropertyList->indexOfTopLevelItem(item) - 1;
    this->Form->ExposedPropertyNames.removeAll(item->text(2));
    QString key = QString("INPUT:%1.%2").arg(item->text(0)).arg(item->text(1));
    this->Form->ToExposeNames.removeAll(key);
    delete item;
    if (row < 0)
    {
      row = 0;
    }

    item = this->Form->PropertyList->topLevelItem(row);
    if (item)
    {
      this->Form->PropertyList->setCurrentItem(item);
    }
    else
    {
      this->updatePropertyButtons(QModelIndex(), QModelIndex());
    }
  }
}

//-----------------------------------------------------------------------------
void pqCustomFilterDefinitionWizard::movePropertyUp()
{
  // Move the selected row up one if possible.
  QTreeWidgetItem* item = this->Form->PropertyList->currentItem();
  if (item)
  {
    int row = this->Form->PropertyList->indexOfTopLevelItem(item);
    if (row > 0)
    {
      this->Form->PropertyList->takeTopLevelItem(row--);
      this->Form->PropertyList->insertTopLevelItem(row, item);
      this->Form->PropertyList->setCurrentItem(item);
    }
  }
}

//-----------------------------------------------------------------------------
void pqCustomFilterDefinitionWizard::movePropertyDown()
{
  // Move the selected row down one if possible.
  QTreeWidgetItem* item = this->Form->PropertyList->currentItem();
  if (item)
  {
    int row = this->Form->PropertyList->indexOfTopLevelItem(item);
    if (row < this->Form->PropertyList->topLevelItemCount() - 1)
    {
      this->Form->PropertyList->takeTopLevelItem(row++);
      this->Form->PropertyList->insertTopLevelItem(row, item);
      this->Form->PropertyList->setCurrentItem(item);
    }
  }
}

//-----------------------------------------------------------------------------
void pqCustomFilterDefinitionWizard::updateInputButtons(
  const QModelIndex& current, const QModelIndex&)
{
  bool indexIsValid = current.isValid();
  this->Form->RemoveInputButton->setEnabled(indexIsValid);
  this->Form->InputUpButton->setEnabled(indexIsValid);
  this->Form->InputDownButton->setEnabled(indexIsValid);
}

//-----------------------------------------------------------------------------
void pqCustomFilterDefinitionWizard::updateOutputButtons(
  const QModelIndex& current, const QModelIndex&)
{
  bool indexIsValid = current.isValid();
  this->Form->RemoveOutputButton->setEnabled(indexIsValid);
  this->Form->OutputUpButton->setEnabled(indexIsValid);
  this->Form->OutputDownButton->setEnabled(indexIsValid);
}

//-----------------------------------------------------------------------------
void pqCustomFilterDefinitionWizard::updatePropertyButtons(
  const QModelIndex& current, const QModelIndex&)
{
  bool indexIsValid = current.isValid();
  this->Form->RemovePropertyButton->setEnabled(indexIsValid);
  this->Form->PropertyUpButton->setEnabled(indexIsValid);
  this->Form->PropertyDownButton->setEnabled(indexIsValid);
}
