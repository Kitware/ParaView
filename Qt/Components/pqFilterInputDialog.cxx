/*=========================================================================

   Program: ParaView
   Module:    pqFilterInputDialog.cxx

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

/// \file pqFilterInputDialog.cxx
/// \date 12/5/2006

#include "pqFilterInputDialog.h"

#include "pqFlatTreeView.h"
#include "pqPipelineFilter.h"
#include "pqPipelineModel.h"
#include "pqServer.h"
#include "pqOutputPort.h"

#include <QButtonGroup>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QList>
#include <QListWidget>
#include <QMap>
#include <QPersistentModelIndex>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QSpacerItem>
#include <QString>
#include <QtDebug>
#include <QVector>

#include "vtkSMInputProperty.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxy.h"


class pqFilterInputDialogItem
{
public:
  pqFilterInputDialogItem();
  pqFilterInputDialogItem(const pqFilterInputDialogItem &other);
  ~pqFilterInputDialogItem();

  pqFilterInputDialogItem &operator=(const pqFilterInputDialogItem &other);

  QList<QPersistentModelIndex> Inputs;
  QList<QPersistentModelIndex> *Invalid;
};


class pqFilterInputDialogInternal
{
public:
  pqFilterInputDialogInternal();
  ~pqFilterInputDialogInternal() {this->clearInputMap();}

  void clearInputMap();
  QString getSourceName(const QModelIndex &index,
      const pqPipelineModel *model) const;
  int getSourceAndPort(const QModelIndex &index, const pqPipelineModel *model,
      pqPipelineSource *&source) const;

  QVector<QWidget *> Widgets;
  QMap<QString, pqFilterInputDialogItem *> InputMap;
  pqFilterInputDialogItem *Current;
};


//----------------------------------------------------------------------------
pqFilterInputDialogItem::pqFilterInputDialogItem()
  : Inputs()
{
  this->Invalid = 0;
}

pqFilterInputDialogItem::pqFilterInputDialogItem(
    const pqFilterInputDialogItem &other)
  : Inputs(other.Inputs)
{
  this->Invalid = 0;

  if(other.Invalid)
    {
    this->Invalid = new QList<QPersistentModelIndex>(*other.Invalid);
    }
}

pqFilterInputDialogItem::~pqFilterInputDialogItem()
{
  if(this->Invalid)
    {
    delete this->Invalid;
    }
}

pqFilterInputDialogItem &pqFilterInputDialogItem::operator=(
    const pqFilterInputDialogItem &other)
{
  this->Inputs = other.Inputs;
  if(this->Invalid && !other.Invalid)
    {
    delete this->Invalid;
    this->Invalid = 0;
    }
  else if(other.Invalid)
    {
    if(this->Invalid)
      {
      *this->Invalid = *other.Invalid;
      }
    else
      {
      this->Invalid = new QList<QPersistentModelIndex>(*other.Invalid);
      }
    }

  return *this;
}


//----------------------------------------------------------------------------
pqFilterInputDialogInternal::pqFilterInputDialogInternal()
  : Widgets(), InputMap()
{
  this->Current = 0;
}

void pqFilterInputDialogInternal::clearInputMap()
{
  this->Current = 0;
  QMap<QString, pqFilterInputDialogItem *>::Iterator iter;
  for(iter = this->InputMap.begin(); iter != this->InputMap.end(); ++iter)
    {
    delete *iter;
    }

  this->InputMap.clear();
}

QString pqFilterInputDialogInternal::getSourceName(const QModelIndex &index,
    const pqPipelineModel *model) const
{
  QString name = model->data(index, Qt::DisplayRole).toString();
  if(model->getTypeFor(index) == pqPipelineModel::OutputPort)
    {
    QModelIndex source = index.parent();
    name.prepend(" - ");
    name.prepend(model->data(source, Qt::DisplayRole).toString());
    }

  return name;
}

int pqFilterInputDialogInternal::getSourceAndPort(const QModelIndex &index,
    const pqPipelineModel *model, pqPipelineSource *&source) const
{
  int port = 0;
  pqServerManagerModelItem *item = model->getItemFor(index);
  source = dynamic_cast<pqPipelineSource *>(item);
  pqOutputPort *outputPort = dynamic_cast<pqOutputPort *>(item);
  if(source)
    {
    port = 0;
    }
  else if(outputPort)
    {
    source = outputPort->getSource();
    port = outputPort->getPortNumber();
    }

  return port;
}


//----------------------------------------------------------------------------
pqFilterInputDialog::pqFilterInputDialog(QWidget *widgetParent)
  : QDialog(widgetParent)
{
  this->Internal = new pqFilterInputDialogInternal();
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
  divider->setFrameShape(QFrame::VLine);
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
  this->Sources->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  baseLayout->addWidget(this->Sources, 1, 1, 2, 1);

  // Create the preview pane and add it to the right.
  this->Preview = new pqFlatTreeView(this);
  this->Preview->setObjectName("Preview");
  this->Preview->getHeader()->hide();
  this->Preview->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  baseLayout->addWidget(this->Preview, 1, 3, 2, 1);

  // Add the separator and the dialog buttons.
  divider = new QFrame(this);
  divider->setFrameShadow(QFrame::Sunken);
  divider->setFrameShape(QFrame::HLine);
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
    pqPipelineFilter *filter,
    const QMap<QString, QList<pqOutputPort*> > &namedInputs)
{
  if(this->Model == model && this->Filter == filter)
    {
    return;
    }

  // Clean up the selection pipeline.
  this->Internal->clearInputMap();
  if(this->Pipeline)
    {
    this->Sources->setModel(0);
    delete this->Pipeline;
    this->Pipeline = 0;
    }

  // Clean up the input property gui elements.
  this->Internal->Widgets.clear();
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
        this->Preview, SLOT(expand(const QModelIndex &)));

    // Make a copy of the model for the user to select sources.
    this->Pipeline = new pqPipelineModel(*this->Model);
    
    this->Pipeline->setEditable(false);
    this->Sources->setModel(this->Pipeline);
    for(int i = 1; i < columns; ++i)
      {
      this->Sources->getHeader()->hideSection(i);
      }
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

    // Listen to the new selection model.
    this->connect(this->Sources->getSelectionModel(),
      SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
      this, SLOT(changeInput(const QItemSelection &, const QItemSelection &)));

    // Select the filter in the preview window, so the user can keep
    // track of it.
    this->Preview->setCurrentIndex(this->Model->getIndexFor(this->Filter));

    // Set the filter title.
    this->setWindowTitle(this->windowTitle() + " - " +
        this->Filter->getSMName());

    // Create a widget to hold all the input properties.
    QWidget *frame = new QWidget();
    frame->setObjectName("InputContainer");
    QGridLayout *frameLayout = new QGridLayout(frame);
    frameLayout->setMargin(0);

    // Add widgets for each of the filter input ports.
    int numInputPorts = this->Filter->getNumberOfInputPorts();
    vtkSMProxy *filterProxy = this->Filter->getProxy();
    for(int port = 0; port < numInputPorts; port++)
      {
      QString name = this->Filter->getInputPortName(port);

      vtkSMInputProperty *input = vtkSMInputProperty::SafeDownCast(
          filterProxy->GetProperty(name.toAscii().data()));
      if(!input)
        {
        qCritical() << "Failed to locate property for input port " << name;
        continue;
        }

      // Save the current list of inputs for the port. The index from
      // the source view is used since it is not modified.
      pqFilterInputDialogItem *inputItem = new pqFilterInputDialogItem();
      QMap<QString, QList<pqOutputPort *> >::ConstIterator inputList =
          namedInputs.find(name);
      if(inputList != namedInputs.end())
        {
        QList<pqOutputPort *>::ConstIterator jter = inputList->begin();
        for( ; jter != inputList->end(); ++jter)
          {
          QPersistentModelIndex itemIndex = this->Pipeline->getIndexFor(*jter);
          if(itemIndex.isValid())
            {
            inputItem->Inputs.append(itemIndex);
            }
          }
        }

      this->Internal->InputMap.insert(name, inputItem);

      QRadioButton *button = new QRadioButton(name, frame);
      button->setObjectName(name);
      button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
      this->InputGroup->addButton(button, port);
      if(port == 0)
        {
        button->setChecked(true);
        }

      if(input->GetMultipleInput())
        {
        QListWidget *list = new QListWidget(frame);
        list->setSelectionMode(QAbstractItemView::NoSelection);
        list->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        frameLayout->addWidget(button, port, 0, Qt::AlignTop);
        frameLayout->addWidget(list, port, 1);
        this->Internal->Widgets.append(list);
        QList<QPersistentModelIndex>::Iterator kter =
            inputItem->Inputs.begin();
        for( ; kter != inputItem->Inputs.end(); ++kter)
          {
          list->addItem(this->Internal->getSourceName(*kter, this->Pipeline));
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
        frameLayout->addWidget(button, port, 0);
        frameLayout->addWidget(label, port, 1);
        this->Internal->Widgets.append(label);
        if(inputItem->Inputs.size() > 0)
          {
          label->setText(this->Internal->getSourceName(inputItem->Inputs[0],
              this->Pipeline));
          if(inputItem->Inputs.size() > 1)
            {
            qDebug() << "More than one input listed for an input port that "
                << "only takes one input.";
            }
          }
        }
      }

    frameLayout->addItem(
        new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding),
        numInputPorts, 0, 1, 2);
    this->InputFrame->setWidget(frame);

    // Make sure the pipeline selection reflects the current input.
    this->changeCurrentInput(0);
    }
}

QList<pqOutputPort *> pqFilterInputDialog::getFilterInputs(
    const QString &port) const
{
  QList<pqOutputPort *> inputList;
  QMap<QString, pqFilterInputDialogItem *>::Iterator iter =
      this->Internal->InputMap.find(port);
  if(iter != this->Internal->InputMap.end())
    {
    QList<QPersistentModelIndex>::Iterator jter = (*iter)->Inputs.begin();
    for( ; jter != (*iter)->Inputs.end(); ++jter)
      {
      pqServerManagerModelItem *item = this->Pipeline->getItemFor(*jter);
      pqPipelineSource *source = dynamic_cast<pqPipelineSource *>(item);
      pqOutputPort *outputPort = dynamic_cast<pqOutputPort *>(item);
      if(source)
        {
        outputPort = source->getOutputPort(0);
        }

      if(outputPort)
        {
        inputList.append(outputPort);
        }
      }
    }

  return inputList;
}

void pqFilterInputDialog::changeCurrentInput(int id)
{
  if(id < 0 || id >= this->Internal->Widgets.size())
    {
    return;
    }

  // Get the input item associated with this button.
  this->Internal->Current = 0;
  QString inputName = this->InputGroup->button(id)->text();
  QMap<QString, pqFilterInputDialogItem *>::Iterator iter =
      this->Internal->InputMap.find(inputName);
  if(iter != this->Internal->InputMap.end())
    {
    this->Internal->Current = *iter;
    }

  // Clear the current selection.
  this->InChangeInput = true; // this will block ::changeInput() from changing
                              // what the user selected for the previous input
                              // port.

  // Clear the previous selectable settings.
  this->Pipeline->setSubtreeSelectable(this->Filter->getServer(), true);
  this->Pipeline->setSubtreeSelectable(this->Filter, false);
  if(this->Internal->Current)
    {
    // Set up the non-selectable input list if it hasn't been done yet.
    if(!this->Internal->Current->Invalid)
      {
      vtkSMInputProperty *input = vtkSMInputProperty::SafeDownCast(
          this->Filter->getProxy()->GetProperty(inputName.toAscii().data()));
      if(input)
        {
        this->Internal->Current->Invalid = new QList<QPersistentModelIndex>();
        QModelIndex root = this->Sources->getRootIndex();
        QModelIndex index = this->Pipeline->getNextIndex(root, root);
        while(index.isValid())
          {
          if(this->Pipeline->isSelectable(index))
            {
            pqPipelineSource *source = 0;
            int port = this->Internal->getSourceAndPort(index, this->Pipeline,
                source);
            if(!source || (source->getNumberOfOutputPorts() > 1 &&
                this->Pipeline->getTypeFor(index) !=
                pqPipelineModel::OutputPort))
              {
              this->Pipeline->setSelectable(index, false);
              }
            else
              {
              input->RemoveAllUncheckedProxies();
              input->AddUncheckedInputConnection(source->getProxy(), port);
              this->Pipeline->setSelectable(index, input->IsInDomains() > 0);
              input->RemoveAllUncheckedProxies();
              }
            }

          index = this->Pipeline->getNextIndex(index, root);
          }
        }
      }

    // Update the selectable settings for the current input.
    if(this->Internal->Current->Invalid)
      {
      foreach (QPersistentModelIndex invalid_index,
        *(this->Internal->Current->Invalid))
        {
        this->Pipeline->setSelectable(invalid_index, false);
        }
      }
    }

  QWidget *widget = this->Internal->Widgets[id];
  QLabel *label = qobject_cast<QLabel *>(widget);
  QListWidget *list = qobject_cast<QListWidget *>(widget);
  if(list)
    {
    this->Sources->setSelectionMode(pqFlatTreeView::ExtendedSelection);
    this->SourcesLabel->setText("Select Source(s)");
    this->MultiHint->show();
    }
  else if(label)
    {
    this->Sources->setSelectionMode(pqFlatTreeView::SingleSelection);
    this->SourcesLabel->setText("Select Source");
    this->MultiHint->hide();
    }


  // Update the selection in the "Source Source" box to reflect the current
  // state.
  QItemSelectionModel *model = this->Sources->getSelectionModel();
  model->clear();
  foreach (QPersistentModelIndex index, this->Internal->Current->Inputs)
    {
    model->select(index, QItemSelectionModel::Select);
    }

  this->InChangeInput = false;
}

void pqFilterInputDialog::changeInput(const QItemSelection &selected,
    const QItemSelection &deselected)
{
  if(this->InChangeInput || !this->Model || !this->Internal->Current)
    {
    return;
    }

  int id = this->InputGroup->checkedId();
  if(id < 0 || id >= this->Internal->Widgets.size())
    {
    return;
    }

  // get the current input display widget.
  QWidget *widget = this->Internal->Widgets[id];
  QLabel *label = qobject_cast<QLabel *>(widget);
  QListWidget *list = qobject_cast<QListWidget *>(widget);

  int port = 0;
  pqPipelineSource *source = 0;
  QList<QListWidgetItem *> items;
  QList<QListWidgetItem *>::Iterator item;
  QModelIndexList indexes = deselected.indexes();
  QModelIndexList::Iterator iter = indexes.begin();
  for( ; iter != indexes.end(); ++iter)
    {
    // Remove the connections for the deselected items.
    source = 0;
    port = this->Internal->getSourceAndPort(*iter, this->Pipeline, source);
    if(source)
      {
      this->Model->removeConnection(source, this->Filter, port);
      }

    // Remove the index from the current input list.
    this->Internal->Current->Inputs.removeAll(*iter);

    // Since the label has only one item, only the list needs to clear
    // the deselected items.
    if(list)
      {
      QString name = this->Internal->getSourceName(*iter, this->Pipeline);
      items = list->findItems(name, Qt::MatchExactly);
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
    source = 0;
    port = this->Internal->getSourceAndPort(*iter, this->Pipeline, source);
    if(source)
      {
      this->Model->addConnection(source, this->Filter, port);
      }

    // Add the index to the current input list.
    this->Internal->Current->Inputs.append(*iter);

    // Add the selected item to the input display widget.
    QString name = this->Internal->getSourceName(*iter, this->Pipeline);
    if(list)
      {
      list->addItem(name);
      }
    else if(label)
      {
      // There should only be one selected item.
      label->setText(name);
      }
    }
}


