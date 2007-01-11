/*=========================================================================

   Program: ParaView
   Module:    pqFilterInputDialog.cxx

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

/// \file pqFilterInputDialog.cxx
/// \date 12/5/2006

#include "pqFilterInputDialog.h"

#include "pqFlatTreeView.h"
#include "pqPipelineBrowserStateManager.h"
#include "pqPipelineFilter.h"
#include "pqPipelineModel.h"
#include "pqServer.h"

#include <QButtonGroup>
#include <QGridLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QList>
#include <QListWidget>
#include <QRadioButton>
#include <QScrollArea>
#include <QSpacerItem>
#include <QString>
#include <QVector>

#include "vtkSMInputProperty.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxy.h"


class pqFilterInputDialogInternal : public QVector<QWidget *> {};


pqFilterInputDialog::pqFilterInputDialog(QWidget *widgetParent)
  : QDialog(widgetParent)
{
  this->Internal = new pqFilterInputDialogInternal();
  this->Manager = new pqPipelineBrowserStateManager(this);
  this->Filter = 0;
  this->Model = 0;
  this->Pipeline = 0;
  this->Sources = 0;
  this->Preview = 0;
  this->InputFrame = 0;
  this->SourcesLabel = 0;
  this->MultiHint = 0;
  this->OkButton = 0;
  this->CancelButton = 0;
  this->InputGroup = new QButtonGroup(this);
  this->InChangeInput = false;

  // Set up the base gui elements.
  QGridLayout *baseLayout = new QGridLayout(this);

  // Add labels for the dialog areas.
  baseLayout->addWidget(new QLabel("Choose Input Port", this), 0, 0);
  this->SourcesLabel = new QLabel("Select Source", this);
  baseLayout->addWidget(this->SourcesLabel, 0, 1);
  QFrame *divider = new QFrame(this);
  divider->setFrameShadow(QFrame::Sunken);
  divider->setFrameShape(QFrame::Shape::VLine);
  baseLayout->addWidget(divider, 0, 2, 3, 1);
  baseLayout->addWidget(new QLabel("Pipeline Preview", this), 0, 3);

  // Add a scroll area in case the input elements don't fit.
  this->InputFrame = new QScrollArea(this);
  this->InputFrame->setObjectName("InputFrame");
  this->InputFrame->setFrameShadow(QFrame::Plain);
  this->InputFrame->setFrameShape(QFrame::NoFrame);
  this->InputFrame->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  this->InputFrame->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  this->InputFrame->setWidgetResizable(true);
  baseLayout->addWidget(this->InputFrame, 1, 0);

  // Add the extended selection hint.
  this->MultiHint = new QLabel(
      "Note: To select multiple sources, use the shift or control key.",
      this);
  this->MultiHint->setWordWrap(true);
  baseLayout->addWidget(this->MultiHint, 2, 0);

  // Create a flat tree view and put it in the middle.
  this->Sources = new pqFlatTreeView(this);
  this->Sources->setObjectName("Sources");
  this->Sources->getHeader()->hide();
  this->Sources->setMaximumWidth(150);
  this->Sources->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
  baseLayout->addWidget(this->Sources, 1, 1, 2, 1);

  // Create the preview pane and add it to the right.
  this->Preview = new pqFlatTreeView(this);
  this->Preview->setObjectName("Preview");
  this->Preview->getHeader()->hide();
  this->Preview->setMaximumWidth(150);
  this->Preview->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
  baseLayout->addWidget(this->Preview, 1, 3, 2, 1);

  // Add the separator and the dialog buttons.
  divider = new QFrame(this);
  divider->setFrameShadow(QFrame::Sunken);
  divider->setFrameShape(QFrame::Shape::HLine);
  baseLayout->addWidget(divider, 3, 0, 1, 4);

  this->OkButton = new QPushButton("&OK", this);
  this->OkButton->setObjectName("OkButton");
  this->OkButton->setDefault(true);
  this->CancelButton = new QPushButton("&Cancel", this);
  this->CancelButton->setObjectName("CancelButton");
  QHBoxLayout *buttonLayout = new QHBoxLayout();
  baseLayout->addLayout(buttonLayout, 4, 0, 1, 4);
  buttonLayout->addStretch();
  buttonLayout->addWidget(this->OkButton);
  buttonLayout->addWidget(this->CancelButton);

  // Connect the dialog buttons to the appropriate action.
  this->connect(this->OkButton, SIGNAL(clicked()), this, SLOT(accept()));
  this->connect(this->CancelButton, SIGNAL(clicked()), this, SLOT(reject()));

  // Listen for a change in the current input property.
  this->connect(this->InputGroup, SIGNAL(buttonClicked(int)),
      this, SLOT(changeCurrentInput(int)));

  this->setWindowTitle("Input Editor");
  this->resize(520, 270);
}

pqFilterInputDialog::~pqFilterInputDialog()
{
  delete this->Internal;
}

void pqFilterInputDialog::setModelAndFilter(pqPipelineModel *model,
    pqPipelineFilter *filter)
{
  if(this->Model == model && this->Filter == filter)
    {
    return;
    }

  // Clean up the selection pipeline.
  if(this->Pipeline)
    {
    this->Sources->setModel(0);
    delete this->Pipeline;
    this->Pipeline = 0;
    }

  // Clean up the input property gui elements.
  this->Internal->clear();
  QList<QAbstractButton *> buttons = this->InputGroup->buttons();
  QList<QAbstractButton *>::Iterator iter = buttons.begin();
  for( ; iter != buttons.end(); ++iter)
    {
    this->InputGroup->removeButton(*iter);
    }

  this->InputFrame->setWidget(0);

  // Save the pipeline model pointer. If the model is null, don't save
  // the filter.
  this->Model = model;
  this->Filter = this->Model ? filter : 0;

  // Add the model to the tree view.
  this->Preview->setModel(this->Model);
  this->Manager->setModelAndView(this->Model, this->Preview);
  if(this->Model)
    {
    // Hide all but the first column.
    int columns = this->Model->columnCount();
    for(int i = 1; i < columns; ++i)
      {
      this->Preview->getHeader()->hideSection(i);
      }

    this->Model->setEditable(false);
    this->connect(this->Model, SIGNAL(firstChildAdded(const QModelIndex &)),
        this->Sources, SLOT(expand(const QModelIndex &)));

    // Make a copy of the model for the user to select sources.
    this->Pipeline = new pqPipelineModel(*this->Model, this);
    this->Pipeline->setEditable(false);
    this->Sources->setModel(this->Pipeline);
    }

  this->setWindowTitle("Input Editor");
  if(this->Filter)
    {
    // Adjust the view's root index to hide the server and items from
    // any other servers.
    pqServer *server = this->Filter->getServer();
    this->Sources->setRootIndex(this->Pipeline->getIndexFor(server));
    this->Sources->expandAll();
    this->Preview->setRootIndex(this->Model->getIndexFor(server));
    this->Preview->expandAll();
    this->Pipeline->setSubtreeSelectable(this->Filter, false);

    // Listen to the new selection model.
    this->connect(this->Sources->getSelectionModel(),
      SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
      this, SLOT(changeInput(const QItemSelection &, const QItemSelection &)));

    // Select the filter in the preview window, so the user can keep
    // track of it.
    this->Preview->setCurrentIndex(this->Model->getIndexFor(this->Filter));

    // Set the filter title.
    this->setWindowTitle(this->windowTitle() + " - " +
        this->Filter->getProxyName());

    // Create a widget to hold all the input properties.
    QWidget *frame = new QWidget();
    frame->setObjectName("InputContainer");
    QGridLayout *frameLayout = new QGridLayout(frame);
    frameLayout->setMargin(0);

    // Make a map of the sources.
    pqPipelineSource *source = 0;
    QMap<vtkSMProxy *, pqPipelineSource *> sourceMap;
    QMap<vtkSMProxy *, pqPipelineSource *>::ConstIterator item;
    for(int index = 0; index < this->Filter->getInputCount(); ++index)
      {
      source = this->Filter->getInput(index);
      sourceMap.insert(source->getProxy(), source);
      }

    // Add widgets for each of the filter inputs.
    vtkSMInputProperty *input = 0;
    vtkSMPropertyIterator *prop =
        this->Filter->getProxy()->NewPropertyIterator();
    int row = 0;
    for(prop->Begin(); !prop->IsAtEnd(); prop->Next())
      {
      input = vtkSMInputProperty::SafeDownCast(prop->GetProperty());
      if(input)
        {
        // TODO: Add support for other input properties such as the
        // 'source' input property of the glyph filter.
        QString name = prop->GetKey();
        if(name != "Input")
          {
          continue;
          }

        QRadioButton *button = new QRadioButton(name, frame);
        button->setObjectName(name);
        button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        this->InputGroup->addButton(button, row);
        if(row == 0)
          {
          button->setChecked(true);
          }

        if(input->GetMultipleInput())
          {
          QListWidget *list = new QListWidget(frame);
          list->setSelectionMode(QAbstractItemView::NoSelection);
          list->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
          frameLayout->addWidget(button, row, 0, Qt::AlignTop);
          frameLayout->addWidget(list, row, 1);
          this->Internal->append(list);
          for(unsigned int i = 0; i < input->GetNumberOfProxies(); ++i)
            {
            item = sourceMap.find(input->GetProxy(i));
            if(item != sourceMap.end())
              {
              source = item.value();
              list->addItem(source->getProxyName());
              }
            }
          }
        else
          {
          QLabel *label = new QLabel(frame);
          label->setFrameShape(QFrame::StyledPanel);
          label->setFrameShadow(QFrame::Sunken);
          label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
          label->setBackgroundRole(QPalette::Base);
          label->setAutoFillBackground(true);
          frameLayout->addWidget(button, row, 0);
          frameLayout->addWidget(label, row, 1);
          this->Internal->append(label);
          item = sourceMap.find(input->GetProxy(0));
          if(item != sourceMap.end())
            {
            source = item.value();
            label->setText(source->getProxyName());
            }
          }

        row++;
        }
      }

    prop->Delete();
    frameLayout->addItem(
        new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding),
        row, 0, 1, 2);
    this->InputFrame->setWidget(frame);

    // Make sure the pipeline selection reflects the current input.
    this->changeCurrentInput(0);
    }
}

void pqFilterInputDialog::getFilterInputPorts(QStringList &ports) const
{
  QList<QAbstractButton *> buttons = this->InputGroup->buttons();
  QList<QAbstractButton *>::Iterator iter = buttons.begin();
  for( ; iter != buttons.end(); ++iter)
    {
    ports.append((*iter)->text());
    }
}

void pqFilterInputDialog::getFilterInputs(const QString &port,
    QStringList &inputs) const
{
  QList<QAbstractButton *> buttons = this->InputGroup->buttons();
  QList<QAbstractButton *>::Iterator iter = buttons.begin();
  for( ; iter != buttons.end(); ++iter)
    {
    if(port == (*iter)->text())
      {
      QWidget *widget = (*this->Internal)[this->InputGroup->id(*iter)];
      QListWidget *list = qobject_cast<QListWidget *>(widget);
      QLabel *label = qobject_cast<QLabel *>(widget);
      if(list)
        {
        for(int row = 0; row < list->count(); ++row)
          {
          inputs.append(list->item(row)->text());
          }
        }
      else if(label)
        {
        inputs.append(label->text());
        }

      break;
      }
    }
}

void pqFilterInputDialog::getCurrentFilterInputs(const QString &port,
    QStringList &inputs) const
{
  if(!this->Filter || port.isEmpty())
    {
    return;
    }

  vtkSMInputProperty *input = vtkSMInputProperty::SafeDownCast(
      this->Filter->getProxy()->GetProperty(port.toAscii().data()));
  if(input)
    {
    // Make a map of the sources.
    pqPipelineSource *source = 0;
    QMap<vtkSMProxy *, pqPipelineSource *> sourceMap;
    QMap<vtkSMProxy *, pqPipelineSource *>::ConstIterator item;
    for(int index = 0; index < this->Filter->getInputCount(); ++index)
      {
      source = this->Filter->getInput(index);
      sourceMap.insert(source->getProxy(), source);
      }

    if(input->GetMultipleInput())
      {
      for(unsigned int i = 0; i < input->GetNumberOfProxies(); ++i)
        {
        item = sourceMap.find(input->GetProxy(i));
        if(item != sourceMap.end())
          {
          source = item.value();
          inputs.append(source->getProxyName());
          }
        }
      }
    else
      {
      item = sourceMap.find(input->GetProxy(0));
      if(item != sourceMap.end())
        {
        source = item.value();
        inputs.append(source->getProxyName());
        }
      }
    }
}

void pqFilterInputDialog::changeCurrentInput(int id)
{
  if(id >= 0 && id < this->Internal->size())
    {
    // Change the selection to match the new input(s).
    this->InChangeInput = true;
    QItemSelectionModel *model = this->Sources->getSelectionModel();
    model->clear();

    // Make a map of the sources.
    pqPipelineSource *source = 0;
    QMap<QString, pqPipelineSource *> sourceMap;
    QMap<QString, pqPipelineSource *>::ConstIterator iter;
    for(int index = 0; index < this->Filter->getInputCount(); ++index)
      {
      source = this->Filter->getInput(index);
      sourceMap.insert(source->getProxyName(), source);
      }

    QWidget *widget = (*this->Internal)[id];
    QLabel *label = qobject_cast<QLabel *>(widget);
    QListWidget *list = qobject_cast<QListWidget *>(widget);
    if(list)
      {
      this->Sources->setSelectionMode(pqFlatTreeView::ExtendedSelection);
      this->SourcesLabel->setText("Select Source(s)");
      this->MultiHint->show();
      for(int row = 0; row < list->count(); ++row)
        {
        QListWidgetItem *item = list->item(row);
        iter = sourceMap.find(item->text());
        if(iter != sourceMap.end())
          {
          model->setCurrentIndex(this->Pipeline->getIndexFor(iter.value()),
              QItemSelectionModel::Select);
          }
        }
      }
    else if(label)
      {
      this->Sources->setSelectionMode(pqFlatTreeView::SingleSelection);
      this->SourcesLabel->setText("Select Source");
      this->MultiHint->hide();
      iter = sourceMap.find(label->text());
      if(iter != sourceMap.end())
        {
        model->setCurrentIndex(this->Pipeline->getIndexFor(iter.value()),
            QItemSelectionModel::Select);
        }
      }

    this->InChangeInput = false;
    }
}

void pqFilterInputDialog::changeInput(const QItemSelection &selected,
    const QItemSelection &deselected)
{
  if(this->InChangeInput || !this->Model)
    {
    return;
    }

  int id = this->InputGroup->checkedId();
  if(id < 0 || id >= this->Internal->size())
    {
    return;
    }

  // get the current input display widget.
  QVariant value;
  QWidget *widget = (*this->Internal)[id];
  QLabel *label = qobject_cast<QLabel *>(widget);
  QListWidget *list = qobject_cast<QListWidget *>(widget);

  pqPipelineSource *source = 0;
  QList<QListWidgetItem *> items;
  QList<QListWidgetItem *>::Iterator item;
  QModelIndexList indexes = deselected.indexes();
  QModelIndexList::Iterator iter = indexes.begin();
  for( ; iter != indexes.end(); ++iter)
    {
    // Remove the connections for the deselected items.
    source = dynamic_cast<pqPipelineSource *>(
        this->Pipeline->getItemFor(*iter));
    this->Model->removeConnection(source, this->Filter);

    // Since the label has only one item, only the list needs to clear
    // the deselected items.
    if(list)
      {
      value = this->Pipeline->data(*iter, Qt::DisplayRole);
      items = list->findItems(value.toString(), Qt::MatchExactly);
      for(item = items.begin(); item != items.end(); ++item)
        {
        delete *item;
        }
      }
    }

  indexes = selected.indexes();
  for(iter = indexes.begin(); iter != indexes.end(); ++iter)
    {
    // Add connections for the newly selected inputs.
    source = dynamic_cast<pqPipelineSource *>(
        this->Pipeline->getItemFor(*iter));
    this->Model->addConnection(source, this->Filter);

    // Add the selected item to the input display widget.
    value = this->Pipeline->data(*iter, Qt::DisplayRole);
    if(list)
      {
      list->addItem(value.toString());
      }
    else if(label)
      {
      // There should only be one selected item.
      label->setText(value.toString());
      }
    }
}


