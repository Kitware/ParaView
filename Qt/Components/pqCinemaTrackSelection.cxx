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
#include <QHeaderView>

#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVLogger.h"
#include "vtkSMSourceProxy.h"

#include "pqAbstractItemSelectionModel.h"
#include "pqApplicationCore.h"
#include "pqCinemaTrack.h"
#include "pqCinemaTrackSelection.h"
#include "pqOutputPort.h"
#include "pqPipelineFilter.h"
#include "pqPipelineModel.h"
#include "pqPropertyWidget.h"
#include "pqRenderView.h"
#include "pqServerManagerModel.h"
#include "pqTreeWidget.h"
#include "pqTreeWidgetItem.h"
#include "ui_pqCinemaTrackSelection.h"

// ============================================================================
/// @brief Concrete implementation for seleciton of proxy arrays.
class pqArraySelectionModel : public pqAbstractItemSelectionModel
{
public:
  pqArraySelectionModel(QObject* parent_ = NULL)
    : pqAbstractItemSelectionModel(parent_)
  {
    this->initializeRootItem();
  };

  ~pqArraySelectionModel() override{};

  void initializeRootItem() override
  {
    pqAbstractItemSelectionModel::RootItem->setData(0, Qt::DisplayRole, "Name");
    pqAbstractItemSelectionModel::RootItem->setData(1, Qt::DisplayRole, "Data Type");
  };

  void populateModel(void* proxy) override
  {
    vtkSMSourceProxy* sourceProxy = static_cast<vtkSMSourceProxy*>(proxy);
    vtkPVDataInformation* dataInfo = sourceProxy->GetDataInformation();
    if (!dataInfo)
    {
      return;
    }

    QList<QTreeWidgetItem*> newItems;
    for (int align = 0; align < 2; align++)
    {
      vtkPVDataSetAttributesInformation const* attribInfo;
      if (align == 0)
      {
        attribInfo = dataInfo->GetPointDataInformation();
      }
      else
      {
        attribInfo = dataInfo->GetCellDataInformation();
      }
      if (!attribInfo)
      {
        continue;
      }

      int numArrays = attribInfo->GetNumberOfArrays();
      for (int i = 0; i < numArrays; i++)
      {
        vtkPVArrayInformation* arrInfo = attribInfo->GetArrayInformation(i);
        if (!arrInfo)
        {
          continue;
        }

        QTreeWidgetItem* item = new QTreeWidgetItem();
        item->setData(0, Qt::DisplayRole, arrInfo->GetName());
        // item->setData(0, Qt::DecorationRole, pixmaps[align]);
        QString dataType = vtkImageScalarTypeNameMacro(arrInfo->GetDataType());
        item->setData(1, Qt::DisplayRole, dataType);
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable);

        // Ignore Normals arrays by default
        Qt::CheckState check =
          QString(arrInfo->GetName()) == QString("Normals") ? Qt::Unchecked : Qt::Checked;
        item->setCheckState(0, check);

        newItems.append(item);
      }
    }
    RootItem->addChildren(newItems);
  };

  QStringList getCheckedItemNames()
  {
    QStringList itemNames;
    int const numItems = this->RootItem->childCount();
    for (int i = 0; i < numItems; i++)
    {
      QModelIndex index_ = pqAbstractItemSelectionModel::index(i, 0, QModelIndex());

      if (pqAbstractItemSelectionModel::data(index_, Qt::CheckStateRole) == Qt::Unchecked)
        continue;

      QString displayedName =
        pqAbstractItemSelectionModel::data(index_, Qt::DisplayRole).toString();
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
  connect(selModel, SIGNAL(currentChanged(QModelIndex const&, QModelIndex const&)), this,
    SLOT(onPipelineItemChanged(QModelIndex const&, QModelIndex const&)));

  // As before, the track selection will be applied to all of the views.  If necessary,
  // the models/tracks can be stored per view to keep selections separate.
  QList<pqRenderViewBase*> views = smModel->findItems<pqRenderViewBase*>();
  if (views.isEmpty() == false)
  {
    plBrowser->setActiveView(views[0]);
  }
  this->initializePipelineItemValues(smModel->findItems<pqPipelineSource*>());
}

// ----------------------------------------------------------------------------
void pqCinemaTrackSelection::initializePipelineItemValues(QList<pqPipelineSource*> const& items)
{
  foreach (pqPipelineSource* plItem, items)
  {
    pqOutputPort const* port = plItem ? plItem->getOutputPort(0) : NULL;

    if (!port)
    {
      vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "failed to query outputPort from index.");
      continue;
    }

    vtkSMSourceProxy* proxy = port->getSourceProxy();
    if (!proxy)
    {
      vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "failed to query proxy!");
      continue;
    }

    ItemValues& values = this->PipelineItemValues[plItem];
    values.first = new pqArraySelectionModel(this);
    values.first->populateModel(proxy);
    values.second = NULL;

    // add only cinema-supported filters
    QWidget* parent_ = this->Ui->wTabValues;
    Qt::WindowFlags parentFlags = parent_->windowFlags();
    const char* className = proxy->GetVTKClassName();
    if (className &&
      (!strcmp(className, "vtkPVContourFilter") || !strcmp(className, "vtkPVMetaSliceDataSet") ||
          !strcmp(className, "vtkPVMetaClipDataSet")))
    {
      pqPipelineFilter* plFilter = static_cast<pqPipelineFilter*>(plItem);
      values.second = new pqCinemaTrack(parent_, parentFlags, plFilter);
      parent_->layout()->addWidget(values.second);
      values.second->hide();
    }
  }
}

// ----------------------------------------------------------------------------
void pqCinemaTrackSelection::onPipelineItemChanged(
  QModelIndex const& current, QModelIndex const& previous)
{
  pqPipelineSource* source = this->getPipelineSource(current);
  pqOutputPort const* port =
    source ? source->getOutputPort(0) : NULL; // qobject_cast<pqOutputPort const*>(smModelItem);

  if (!port)
  {
    vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "failed to query outputPort from index(%d, %d)",
      current.row(), current.column());

    // TODO create a functions which sets a null model and disables the widgets
    this->Ui->viewArrayPicker->setModel(NULL);
    this->Ui->tabProxyProperties->setEnabled(false);
    return;
  }

  // set current item's pipeline model
  ItemValuesMap::iterator valuesIt = this->PipelineItemValues.find(source);
  if (valuesIt == this->PipelineItemValues.end())
  {
    return;
  }

  this->Ui->viewArrayPicker->setModel(valuesIt->second.first);
  this->Ui->tabProxyProperties->setEnabled(true);
  QHeaderView* header = this->Ui->viewArrayPicker->header();
  header->resizeSections(QHeaderView::ResizeToContents);
  header->setStretchLastSection(true);

  // set current item's value input widget if available
  // show the current
  pqCinemaTrack* track = valuesIt->second.second;
  if (track)
  {
    track->show();
    this->Ui->tabProxyProperties->setTabEnabled(1, true);

    // array selection is disabled, so set jump to the 'controls' tab
    if (!this->Ui->tabProxyProperties->isTabEnabled(0))
    {
      this->Ui->tabProxyProperties->setCurrentIndex(1);
    }
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

  ItemValuesMap::iterator prevValuesIt = this->PipelineItemValues.find(prevItem);
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
void pqCinemaTrackSelection::enableArraySelection(bool enable)
{
  this->Ui->tabProxyProperties->setTabEnabled(0, enable);

  // force item selection changed to update the active tab
  pqPipelineBrowserWidget* plBrowser = this->Ui->wPipelineBrowser;
  QItemSelectionModel* selModel = plBrowser->getSelectionModel();
  this->onPipelineItemChanged(selModel->currentIndex(), QModelIndex());
}

// ----------------------------------------------------------------------------
pqPipelineSource* pqCinemaTrackSelection::getPipelineSource(QModelIndex const& index) const
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

  ItemValuesMap const& valuesMap = this->PipelineItemValues;
  for (ItemValuesMap::const_iterator it = valuesMap.begin(); it != valuesMap.end(); it++)
  {
    if (pqCinemaTrack* track = qobject_cast<pqCinemaTrack*>((*it).second.second))
    {
      // Update names as they might be out of sync (e.g. if the user renamed a filter).
      // TODO: Update the name right away when it changes with an ss connection.
      track->setFilterName(QString((*it).first->getSMName()));
      tracks.append(track);
    }
  }

  return tracks;
}

//------------------------------------------------------------------------------
QString pqCinemaTrackSelection::getTrackSelectionAsString(QString const& format)
{
  QString cinema_tracks;
  QList<pqCinemaTrack*> tracks = this->getTracks();

  for (int index = 0; index < tracks.count(); index++)
  {
    pqCinemaTrack* const& p = tracks.at(index);
    if (!p->explore())
    {
      continue;
    }

    QString name = p->filterName();
    QString values = "[";
    QVariantList vals = p->scalars();
    for (int j = 0; j < vals.count(); j++)
    {
      bool ok = true;
      QString valueStr = QString::number(vals.value(j).toDouble(&ok));
      if (!ok)
      {
        continue;
      }

      values += valueStr;
      values += ",";
    }
    values.chop(1);
    values += "]";

    // Following the smtrace.py convention, the first letter of the UI name is changed
    // to lower-case to facilitate passing these values to a CoProcessing pipeline.
    // When exporting through menu->export the actual UI name is resolved in pv_introspect.
    name = name.left(1).toLower() + name.mid(1);

    QString info = format.arg(name).arg(values);
    cinema_tracks += info;

    // append a comma except to the last element
    if (index < tracks.count() - 1)
      cinema_tracks += ", ";
  }

  return cinema_tracks;
}

// -----------------------------------------------------------------------------
QString pqCinemaTrackSelection::getArraySelectionAsString(QString const& format)
{
  bool allArraysUnchecked = true;
  QString array_selection("");
  ItemValuesMap const& valuesMap = this->PipelineItemValues;
  for (ItemValuesMap::const_iterator it = valuesMap.begin(); it != valuesMap.end(); it++)
  {
    pqArraySelectionModel* model = (*it).second.first;
    if (model)
    {
      QString itemName = QString((*it).first->getSMName());
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

        // Following the smtrace.py convention, the first letter of the UI name is changed
        // to lower-case to facilitate passing these values to a CoProcessing pipeline.
        // When exporting through menu->export the actual UI name is resolved in pv_introspect.
        itemName = itemName.left(1).toLower() + itemName.mid(1);

        QString info = format.arg(itemName).arg(values);
        array_selection += info;
        array_selection += ", ";
        allArraysUnchecked = false;
      }
    }
  }

  if (!allArraysUnchecked)
  {
    // chop off the last ", "
    array_selection.chop(2);
  }

  return array_selection;
}
