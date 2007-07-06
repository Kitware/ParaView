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


class pqFilterInputDialogInternal
{
public:
  pqFilterInputDialogInternal();
  ~pqFilterInputDialogInternal() {}

  QVector<QWidget *> Widgets;
  QMap<QString, pqPipelineSource *> SourceMap;

  QMap<QString, QList<pqOutputPort*> > Inputs;
};

//----------------------------------------------------------------------------
class pqFilterInputPipelineModel : public pqPipelineModel
{
  typedef pqPipelineModel Superclass;
public:
  /// \brief
  ///   Makes a copy of a pipeline model.
  /// \param other The pipeline model to copy.
  /// \param parent The parent object.
  pqFilterInputPipelineModel(const pqPipelineModel &other, pqFilterInputDialog *parent=0)
    : Superclass(other, parent)
    {
    this->Dialog = parent;
    }

  /// Overridden to ensure that only those items are selectable which can be set
  /// as the input to the filter i.e. their output data type  matches what is
  /// expected at the input of the filter.
  virtual Qt::ItemFlags flags(const QModelIndex &idx) const
    {
    Qt::ItemFlags cur_flags = this->Superclass::flags(idx);
    if ( (cur_flags & Qt::ItemIsSelectable) > 0)
      {
      // Ensure that the source produces data as expected by the current input
      // port.
      pqServerManagerModelItem* item = this->getItemFor(idx);
      pqPipelineSource* source = qobject_cast<pqPipelineSource*>(item);
      pqOutputPort* opport = source? source->getOutputPort(0) :
        qobject_cast<pqOutputPort*>(item);
      if (opport)
        {
        if (!this->Dialog->isInputAcceptable(opport))
          {
          cur_flags &= ~Qt::ItemIsSelectable;
          cur_flags &= ~Qt::ItemIsEnabled;
          }
        }
      }
    return cur_flags;
    }

protected:
  pqFilterInputDialog* Dialog;

};

//----------------------------------------------------------------------------
pqFilterInputDialogInternal::pqFilterInputDialogInternal()
  : Widgets(), SourceMap()
{
}


//----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
void pqFilterInputDialog::setModelAndFilter(pqPipelineModel *model,
    pqPipelineFilter *filter,
    const QMap<QString, QList<pqOutputPort*> >& namedInputs)
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
  this->Internal->Widgets.clear();
  this->Internal->SourceMap.clear();
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
        this->Preview, SLOT(expand(const QModelIndex &)));

    // Make a copy of the model for the user to select sources.
    this->Pipeline = new pqFilterInputPipelineModel(*this->Model, this);
    
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
        this->Filter->getSMName());

    // Create a widget to hold all the input properties.
    QWidget *frame = new QWidget();
    frame->setObjectName("InputContainer");
    QGridLayout *frameLayout = new QGridLayout(frame);
    frameLayout->setMargin(0);

    // Add widgets for each of the filter input ports.
    int num_input_ports = this->Filter->getNumberOfInputPorts();
    vtkSMProxy* filterProxy = this->Filter->getProxy();
    
    for (int cc=0; cc < num_input_ports; cc++)
      {
      QString name = this->Filter->getInputPortName(cc);

      vtkSMInputProperty *input = vtkSMInputProperty::SafeDownCast(
        filterProxy->GetProperty(name.toAscii().data()));
      if(!input)
        {
        qCritical() << "Failed to locate property for input port "
          << name;
        continue;
        }

      // What is the current state of the inputs for this port?
      QMap<QString, QList<pqOutputPort*> >::const_iterator mapIter =
        namedInputs.find(name);
      const QList<pqOutputPort*> inputs = 
        mapIter != namedInputs.end()? mapIter.value() : QList<pqOutputPort*>();
      this->Internal->Inputs[name] = inputs;

      QRadioButton *button = new QRadioButton(name, frame);
      button->setObjectName(name);
      button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
      this->InputGroup->addButton(button, cc);
      if(cc == 0)
        {
        button->setChecked(true);
        }

      if(input->GetMultipleInput())
        {
        QListWidget *list = new QListWidget(frame);
        list->setSelectionMode(QAbstractItemView::NoSelection);
        list->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        frameLayout->addWidget(button, cc, 0, Qt::AlignTop);
        frameLayout->addWidget(list, cc, 1);
        this->Internal->Widgets.append(list);
        foreach (pqOutputPort* opport, inputs)
          {
          QModelIndex itemIndex = model->getIndexFor(opport);
          QVariant label = model->data(itemIndex, Qt::DisplayRole); 
          QListWidgetItem* lwItem = new QListWidgetItem(list);
          lwItem->setText(label.toString());
          lwItem->setData(Qt::UserRole, opport);
          list->addItem(lwItem);
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
        frameLayout->addWidget(button, cc, 0);
        frameLayout->addWidget(label, cc, 1);
        this->Internal->Widgets.append(label);

        if (inputs.size() > 0)
          {
          QModelIndex itemIndex = model->getIndexFor(inputs[0]);
          QVariant text = model->data(itemIndex, Qt::DisplayRole); 
          label->setText(text.toString());
          }
        }
      }

    frameLayout->addItem(
        new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding),
        num_input_ports, 0, 1, 2);
    this->InputFrame->setWidget(frame);

    // Make sure the pipeline selection reflects the current input.
    this->changeCurrentInput(0);
    }
}

//-----------------------------------------------------------------------------
QList<pqOutputPort*>& pqFilterInputDialog::getFilterInputs(
  const QString &port) const
{
  return this->Internal->Inputs[port];
}

//-----------------------------------------------------------------------------
void pqFilterInputDialog::changeCurrentInput(int id)
{
  if (id<0 || id >= this->Filter->getNumberOfInputPorts() || 
    id >= this->Internal->Widgets.size())
    {
    return;
    }

  // We update the selection in model (on which the user makes the selection for
  // the input) to reflect the currently choosen inputs for the given port.

  QString inputPortName = this->Filter->getInputPortName(id);
  QList<pqOutputPort*>& inputs = this->Internal->Inputs[inputPortName];

  // Change the selection to match the new input(s).
  this->InChangeInput = true;

  QItemSelectionModel *model = this->Sources->getSelectionModel();
  model->clear();

  foreach (pqOutputPort* input, inputs)
    {
    model->setCurrentIndex(this->Pipeline->getIndexFor(input),
      QItemSelectionModel::Select);
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

  // Update selectability of all the sources in the Model based on the input
  // port domain.
  vtkSMInputProperty* ivp = 
    vtkSMInputProperty::SafeDownCast(this->Filter->getProxy()->GetProperty(
        inputPortName.toAscii().data()));
  ivp->RemoveAllUncheckedProxies();
  


  this->InChangeInput = false;
}

//-----------------------------------------------------------------------------
void pqFilterInputDialog::changeInput(const QItemSelection &selected,
    const QItemSelection &deselected)
{
  if(this->InChangeInput || !this->Model)
    {
    return;
    }

  int id = this->InputGroup->checkedId();
  if(id < 0 || id >= this->Internal->Widgets.size())
    {
    return;
    }

  // Update the this->Internal->Inputs data structure to reflect the current
  // choice of inputs.
  QList<pqOutputPort*> & inputs = this->Internal->Inputs[
    this->Filter->getInputPortName(id)];

  QList<pqOutputPort*> added;
  QList<pqOutputPort*> removed;

  // Remove all deselected items from the inputs.
  foreach (QModelIndex index, deselected.indexes())
    {
    pqServerManagerModelItem* smItem = this->Pipeline->getItemFor(index);
    pqPipelineSource* source = qobject_cast<pqPipelineSource*>(smItem);
    pqOutputPort* opPort = source? source->getOutputPort(0) : 
      qobject_cast<pqOutputPort*>(smItem);
    if (opPort && !removed.contains(opPort))
      {
      removed.push_back(opPort);
      }
    inputs.removeAll(opPort);
    }

  // Add newly selected items to the inputs.
  foreach (QModelIndex index, selected.indexes())
    {
    pqServerManagerModelItem* smItem = this->Pipeline->getItemFor(index);
    pqPipelineSource* source = qobject_cast<pqPipelineSource*>(smItem);
    pqOutputPort* opPort = source? source->getOutputPort(0) : 
      qobject_cast<pqOutputPort*>(smItem);
    if (opPort && !added.contains(opPort))
      {
      added.push_back(opPort);
      }
    if (opPort && !inputs.contains(opPort))
      {
      inputs.push_back(opPort);
      }
    }

  // Now update the "preview" model to reflect the new connections.
  foreach (pqOutputPort* opport, removed)
    {
    this->Model->removeConnection(opport->getSource(), this->Filter,
      opport->getPortNumber());
    }

  foreach (pqOutputPort* opport, added)
    {
    this->Model->addConnection(opport->getSource(), this->Filter,
      opport->getPortNumber());
    }

  // Now update the widget that shows the current value of the input property.
  QWidget *widget = this->Internal->Widgets[id];
  QLabel *label = qobject_cast<QLabel *>(widget);
  QListWidget *list = qobject_cast<QListWidget *>(widget);

  if (list)
    {
    list->clear();
    foreach (pqOutputPort* opport, inputs)
      {
      QModelIndex itemIndex = this->Pipeline->getIndexFor(opport);
      QVariant text = this->Pipeline->data(itemIndex, Qt::DisplayRole); 
      QListWidgetItem* lwItem = new QListWidgetItem(list);
      lwItem->setText(text.toString());
      lwItem->setData(Qt::UserRole, opport);
      list->addItem(lwItem);
      }
    }
  else 
    {
    QModelIndex itemIndex = this->Pipeline->getIndexFor(inputs[0]);
    QVariant text = this->Pipeline->data(itemIndex, Qt::DisplayRole); 
    label->setText(text.toString());
    }
}

//-----------------------------------------------------------------------------
bool pqFilterInputDialog::isInputAcceptable(pqOutputPort* opport) const
{
  int id = this->InputGroup->checkedId();
  QString ipPortName = this->Filter->getInputPortName(id);
  vtkSMInputProperty* ip = vtkSMInputProperty::SafeDownCast(
    this->Filter->getProxy()->GetProperty(ipPortName.toAscii().data()));
  if (!ip)
    {
    return false;
    }

  ip->RemoveAllUncheckedProxies();
  ip->AddUncheckedInputConnection(opport->getSource()->getProxy(),
    opport->getPortNumber());
  bool in_domains = (ip->IsInDomains()>0);
  ip->RemoveAllUncheckedProxies();
  return in_domains;
}


