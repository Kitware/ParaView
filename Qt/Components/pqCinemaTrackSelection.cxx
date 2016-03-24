/*=========================================================================

   Program: ParaView
   Module:  pqCinemaTrackSelection.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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
#include <QDebug>

#include "vtkSMSourceProxy.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVArrayInformation.h"

#include "pqCinemaTrackSelection.h"
#include "ui_pqCinemaTrackSelection.h"
#include "pqPropertyWidget.h"
#include "pqPipelineFilter.h"
#include "pqCinemaTrack.h"
#include "pqApplicationCore.h"
#include "pqServerManagerModel.h"
#include "pqOutputPort.h"
#include "pqPipelineModel.h"
#include "pqRenderView.h"
#include "pqTreeWidget.h"
#include "pqTreeWidgetItem.h"
#include "pqTreeWidgetCheckHelper.h"
#include "pqAbstractItemSelectionModel.h"


// ============================================================================
/// @brief Concrete implementation for seleciton of proxy arrays.
class pqArraySelectionModel : public pqAbstractItemSelectionModel
{
public:

  pqArraySelectionModel(QObject* parent = NULL)
  : pqAbstractItemSelectionModel(parent)
  {
    this->initializeRootItem();
  };

  ~pqArraySelectionModel()
  {
  };

  void initializeRootItem()
  {
    pqAbstractItemSelectionModel::RootItem->setData(0, Qt::DisplayRole, "Name");
    pqAbstractItemSelectionModel::RootItem->setData(1, Qt::DisplayRole, "Data Type");
  };

  void populateModel(void* proxy)
  {
    vtkSMSourceProxy* sourceProxy = static_cast<vtkSMSourceProxy*>(proxy);
    vtkPVDataInformation* dataInfo = sourceProxy->GetDataInformation();
    if (!dataInfo)
      {
      return;
      }

    vtkPVDataSetAttributesInformation const* attribInfo = dataInfo->GetPointDataInformation();
    if (!attribInfo)
      {
      return;
      }

    int const numArrays = attribInfo->GetNumberOfArrays();
    QList<QTreeWidgetItem*> newItems;
    for (int i = 0 ; i < numArrays ; i++)
      {
      vtkPVArrayInformation* arrInfo = attribInfo->GetArrayInformation(i);
      if (!arrInfo)
        {
        continue;
        }

      QTreeWidgetItem* item = new QTreeWidgetItem();
      item->setData(0, Qt::DisplayRole, arrInfo->GetName());
      //item->setData(0, Qt::DecorationRole, pixmaps[k]);
      QString dataType = vtkImageScalarTypeNameMacro(arrInfo->GetDataType());
      item->setData(1, Qt::DisplayRole, dataType);
      item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable);

      // Ignore Normals arrays by default
      Qt::CheckState check = QString(arrInfo->GetName()) == QString("Normals") ?
        Qt::Unchecked : Qt::Checked;
      item->setCheckState(0, check);

      newItems.append(item);
      }
    RootItem->addChildren(newItems);
  };

  QStringList getCheckedItemNames()
  {
    QStringList itemNames;
    int const numItems = this->RootItem->childCount();
    for (int i = 0; i < numItems; i++)
      {
        QTreeWidgetItem* item = this->RootItem->child(i);
        QModelIndex index = pqAbstractItemSelectionModel::index(i, 0, QModelIndex());

        if (pqAbstractItemSelectionModel::data(index, Qt::CheckStateRole) == Qt::Unchecked)
          continue;

        QString displayedName = pqAbstractItemSelectionModel::data(index, Qt::DisplayRole).toString();
        itemNames.append(displayedName);
      }

    return itemNames;
  };
};


// ============================================================================
pqCinemaTrackSelection::pqCinemaTrackSelection(QWidget* parent_)
: QWidget(parent_)
, Ui(new Ui::CinemaTrackSelection())
{
  this->Ui->setupUi(this);
  this->Ui->viewArrayPicker->setRootIsDecorated(false);
}

// ----------------------------------------------------------------------------
pqCinemaTrackSelection::~pqCinemaTrackSelection()
{
  delete Ui;
}

// ----------------------------------------------------------------------------
void pqCinemaTrackSelection::initializePipelineBrowser()
{
  pqServerManagerModel* smModel = pqApplicationCore::instance()->getServerManagerModel();
  pqPipelineBrowserWidget* plBrowser = this->Ui->wPipelineBrowser;
  pqPipelineModel* model = new pqPipelineModel(*smModel, plBrowser);
  plBrowser->setModel(model);
  plBrowser->expandAll();

  QItemSelectionModel* selModel = plBrowser->getSelectionModel();
  connect(selModel, SIGNAL(currentChanged(QModelIndex const &, QModelIndex const &)),
    this, SLOT(onPipelineItemChanged(QModelIndex const &, QModelIndex const &)));

  // As before, the track selection will be applied to all of the views.  If necessary,
  // the models/tracks can be stored per view to keep selections separate.
  QList<pqRenderViewBase*> views = smModel->findItems<pqRenderViewBase*>();
  plBrowser->setActiveView(views[0]);

  this->initializePipelineItemValues(smModel->findItems<pqPipelineSource*>());
}

// ----------------------------------------------------------------------------
void pqCinemaTrackSelection::initializePipelineItemValues(QList<pqPipelineSource*>
   const & items)
{
  foreach(pqPipelineSource* plItem, items)
    {
    pqOutputPort const* port = plItem ? plItem->getOutputPort(0) : NULL;

    if (!port)
      {
      PV_DEBUG_PANELS() << "Failed to query outputPort from index(";
      continue;
      }

    vtkSMSourceProxy* proxy = port->getSourceProxy();
    if (!proxy)
      {
      PV_DEBUG_PANELS() << "Failed to query proxy!";
      continue;
      }

    ItemValues & values = this->PipelineItemValues[QString(plItem->getSMName())];
    values.first = new pqArraySelectionModel(this);
    values.first->populateModel(proxy);
    values.second = NULL;

    // add only cinema-supported filters
    QWidget* parent = this->Ui->wTabValues;
    Qt::WindowFlags parentFlags = parent->windowFlags();
    if (!strcmp(proxy->GetVTKClassName(), "vtkPVContourFilter")    ||
        !strcmp(proxy->GetVTKClassName(), "vtkPVMetaSliceDataSet") ||
        !strcmp(proxy->GetVTKClassName(), "vtkPVMetaClipDataSet"))
      {
      pqPipelineFilter* plFilter = static_cast<pqPipelineFilter*>(plItem);
      values.second = new pqCinemaTrack(parent, parentFlags, plFilter);
      parent->layout()->addWidget(values.second);
      values.second->hide();
      }
    }
}

// ----------------------------------------------------------------------------
void pqCinemaTrackSelection::onPipelineItemChanged(QModelIndex const & current,
  QModelIndex const & previous)
{
  pqPipelineSource* source = this->getPipelineSource(current);
  pqOutputPort const* port = source ? source->getOutputPort(0) : NULL;  // qobject_cast<pqOutputPort const*>(smModelItem);

  if (!port)
    {
    PV_DEBUG_PANELS() << "Failed to query outputPort from index(" << current.row()
      << ", " << current.column() << ")";

    // TODO create a functions which sets a null model and disables the widgets
    this->Ui->viewArrayPicker->setModel(NULL);
    this->Ui->tabProxyProperties->setEnabled(false);
    return;
    }

  //set current item's pipeline model
  vtkSMSourceProxy* proxy = port->getSourceProxy();
  ItemValuesMap::iterator valuesIt = this->PipelineItemValues.find(QString(source->getSMName()));
  if (valuesIt == this->PipelineItemValues.end())
    {
    return;
    }

  this->Ui->viewArrayPicker->setModel(valuesIt->second.first);
  this->Ui->tabProxyProperties->setEnabled(true);
  QHeaderView* header = this->Ui->viewArrayPicker->header();
  header->resizeSections(QHeaderView::ResizeToContents);
  header->setStretchLastSection(true);

  //set current item's value input widget if available
  // show the current
  pqCinemaTrack* track = valuesIt->second.second;
  if (track)
    {
    track->show();
    this->Ui->tabProxyProperties->setTabEnabled(1, true);
    }
  else
    {
    this->Ui->tabProxyProperties->setCurrentIndex(0);
    this->Ui->tabProxyProperties->setTabEnabled(1, false);
    }
  // hide the previous
  pqPipelineSource* prevItem = this->getPipelineSource(previous);
  if (!prevItem)
    {
      return;
    }

  ItemValuesMap::iterator prevValuesIt = this->PipelineItemValues.find(QString(prevItem->getSMName()));
  if (prevValuesIt != this->PipelineItemValues.end())
    {
    pqCinemaTrack* prevTrack = prevValuesIt->second.second;
    if (prevTrack)
      {
      prevTrack->hide();
      }
    }
}

// ----------------------------------------------------------------------------
pqPipelineSource* pqCinemaTrackSelection::getPipelineSource(QModelIndex const & index) const
{
  if (!index.isValid())
    {
    return NULL;
    }

  // transform the FilterModel QMIndex to its PipelineModel equivalent
  pqPipelineModel const* model = this->Ui->wPipelineBrowser->getPipelineModel(index);
  if (!model)
    {
    return NULL;
    }

  QModelIndex const plModelIndex = this->Ui->wPipelineBrowser->pipelineModelIndex(index);
  // query an item from the PipelineModel and get its vtkSMSourceProxy
  pqServerManagerModelItem* smModelItem = model->getItemFor(plModelIndex);
  pqPipelineSource* source = qobject_cast<pqPipelineSource*>(smModelItem);

  return source;
}

//-----------------------------------------------------------------------------
QList<pqCinemaTrack*> pqCinemaTrackSelection::getTracks()
{
  QList<pqCinemaTrack*> tracks;

  ItemValuesMap const & valuesMap = this->PipelineItemValues;  
  for (ItemValuesMap::const_iterator it = valuesMap.begin(); it != valuesMap.end(); it++)
    {
    if (pqCinemaTrack* track = qobject_cast<pqCinemaTrack*>((*it).second.second))
      {
      tracks.append(track);
      }
    }

  return tracks;
}

//------------------------------------------------------------------------------
QString pqCinemaTrackSelection::getTrackSelectionAsString(QString const & format)
{
  if (!this->Ui->gbTrackSelection->isChecked())
    {
    // An empty string will be handled by the cinema python scripts as default values
    return QString();
    }

  QString cinema_tracks;
  QList<pqCinemaTrack*> tracks = this->getTracks();

  for (int index = 0; index < tracks.count(); index++)
    {
    pqCinemaTrack* const & p = tracks.at(index);
    if (!p->explore())
      {
      continue;
      }

    QString name = p->filterName();
    QString values = "[";
    QVariantList vals = p->scalars();
    for (int j = 0; j < vals.count(); j++)
      {
      values += QString::number(vals.value(j).toDouble());
      values += ",";
      }
    values.chop(1);
    values += "]";
    QString info = format.arg(name).arg(values);
    cinema_tracks+= info;

    // append a comma except to the last element
    if (index < tracks.count() - 1)
      cinema_tracks += ", ";
    }

  return cinema_tracks;
}

// -----------------------------------------------------------------------------
QString pqCinemaTrackSelection::getArraySelectionAsString(QString const & format)
{
  if (!this->Ui->gbTrackSelection->isChecked())
    {
    // An empty string will be handled by the cinema python scripts as default values
    return QString();
    }

  QString array_selection("'arraySelection' : {");
  ItemValuesMap const & valuesMap = this->PipelineItemValues;
  for (ItemValuesMap::const_iterator it = valuesMap.begin(); it != valuesMap.end(); it++)
    {
    pqArraySelectionModel* model = (*it).second.first;
    if (model)
      {
      QString const & pipelineItemName = (*it).first;
      QStringList arrayNames = model->getCheckedItemNames();

      // append to the selection string
      if (arrayNames.count() > 0)
        {
        QString values = "[";
        for (int j = 0; j < arrayNames.count(); j++)
          {
          values += QString("'") + arrayNames.at(j) + QString("'");
          if (j < arrayNames.count() - 1)
            values += ", ";
          }
        values += "]";
        QString info = format.arg(pipelineItemName).arg(values);
        array_selection += info;
        array_selection += ", ";
        }
      }
    }
  // chop off the last ", "
  array_selection.chop(2);
  array_selection += "}";

  return array_selection;
}
