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
    for (int i = 0 ; i < numArrays ; i++)
      {
      vtkPVArrayInformation* arrInfo = attribInfo->GetArrayInformation(i);
      if (!arrInfo)
        {
        continue;
        }
      
      QList<QTreeWidgetItem*> newItems;
      QTreeWidgetItem* item = new QTreeWidgetItem();
      item->setData(0, Qt::DisplayRole, arrInfo->GetName());
      //item->setData(0, Qt::DecorationRole, pixmaps[k]);
      QString dataType = vtkImageScalarTypeNameMacro(arrInfo->GetDataType());
      item->setData(1, Qt::DisplayRole, dataType);
      item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable);
      //qDebug() << "->>> Array " << i << ": " << QString(arrInfo->GetName());

      item->setCheckState(0, Qt::Checked);
      newItems.append(item);

      RootItem->addChildren(newItems);
      }
  };

  QString getModelSelection()
  {
    return QString();
  };
};


// ============================================================================
pqCinemaTrackSelection::pqCinemaTrackSelection(QWidget* parent_)
: QWidget(parent_)
, Ui(new Ui::CinemaTrackSelection())
{
  this->Ui->setupUi(this);
  this->initializePipelineBrowser();

  // view's decoration
  this->Ui->viewArrayPicker->setRootIsDecorated(false);

  connect(this->Ui->pbPrevious, SIGNAL(clicked()), this, SLOT(onPreviousClicked()));
  connect(this->Ui->pbNext, SIGNAL(clicked()), this, SLOT(onNextClicked()));

  // TODO Connect selection changed (selectionModel) to checkbox toggling
  this->Ui->wOldSelection->hide();
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

  // pqPipelineBrowser takes ownership of the model
  pqPipelineModel* model = new pqPipelineModel(*smModel, NULL);
  pqPipelineBrowserWidget* plBrowser = this->Ui->wPipelineBrowser;
  plBrowser->setModel(model);
  plBrowser->expandAll();

  QItemSelectionModel* selModel = plBrowser->getSelectionModel();
  connect(selModel, SIGNAL(currentChanged(QModelIndex const &, QModelIndex const &)),
    this, SLOT(onPipelineItemChanged(QModelIndex const &, QModelIndex const &)));

  // TODO change the view depending on the wViewSelection FIXME
  QList<pqRenderViewBase*> views = smModel->findItems<pqRenderViewBase*>();
  plBrowser->setActiveView(views[0]);

  this->initializePipelineItemValues(smModel->findItems<pqPipelineSource*>());
}

void pqCinemaTrackSelection::initializePipelineItemValues(QList<pqPipelineSource*> const & items)
{
  foreach(pqPipelineSource* plItem, items)
    {
    pqOutputPort const* port = plItem ? plItem->getOutputPort(0) : NULL;

    if (!port)
      {
      qDebug() << "Failed to query outputPort from index(";
      continue;
      }

    vtkSMSourceProxy* proxy = port->getSourceProxy();
    if (!proxy)
      {
      qDebug() << "Failed to query proxy!";
      continue;
      }

    ItemValues & values = this->PipelineItemValues[QString(plItem->getSMName())];
    values.first = new pqArraySelectionModel(this);
    values.first->populateModel(proxy);
    values.second = NULL;

    // add only cinema-supported filters    
    Qt::WindowFlags parentFlags = this->Ui->swTracks->windowFlags();
    if (!strcmp(proxy->GetVTKClassName(), "vtkPVContourFilter")    ||
        !strcmp(proxy->GetVTKClassName(), "vtkPVMetaSliceDataSet") ||
        !strcmp(proxy->GetVTKClassName(), "vtkPVMetaClipDataSet"))
      {
      QWidget* parent = this->Ui->wTabValues;
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
  pqOutputPort const* port = source ? source->getOutputPort(0) : NULL;
   // qobject_cast<pqOutputPort const*>(smModelItem);

  // TODO review why using here the port and not on init, is this check necessary???
  if (!port)
    {
    PV_DEBUG_PANELS() << "Failed to query outputPort from index(" << current.row()
      << ", " << current.column() << ")";

    // TODO create a functions which sets a null model and disables the widgets
    this->Ui->viewArrayPicker->setModel(NULL);
    this->Ui->tabProxyProperties->setEnabled(false);

    return;
    }

  vtkSMSourceProxy* proxy = port->getSourceProxy();
  //proxy->PrintSelf(std::cout, vtkIndent());
  ItemValuesMap::iterator valuesIt = this->PipelineItemValues.find(QString(source->getSMName()));
  if (valuesIt == this->PipelineItemValues.end())
    {
    return;
    }
  
  // set the current model
  this->Ui->viewArrayPicker->setModel(valuesIt->second.first);
  this->Ui->tabProxyProperties->setEnabled(true);
  QHeaderView* header = this->Ui->viewArrayPicker->header();
  header->resizeSections(QHeaderView::ResizeToContents);
  header->setStretchLastSection(true);


////////////////// update track value input widget //////////////////////
  //hide the previous
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
  this->Ui->wTabValues->setEnabled(false);

  // set the current value input widget if any
  pqCinemaTrack* track = valuesIt->second.second;
  if (track)
    {
    //update the current one
    track->show();
    this->Ui->wTabValues->setEnabled(true);
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

// ----------------------------------------------------------------------------
void pqCinemaTrackSelection::onPreviousClicked()
{
  int previousIndex = this->Ui->swTracks->currentIndex() - 1;
  if (previousIndex >= 0)
    {
    this->Ui->swTracks->setCurrentIndex(previousIndex);
    if (previousIndex == 0) // first track
      {
      this->Ui->pbPrevious->setEnabled(false);
      }

    if (!this->Ui->pbNext->isEnabled())
      {
      this->Ui->pbNext->setEnabled(true);
      }
    }
}

// ----------------------------------------------------------------------------
void pqCinemaTrackSelection::onNextClicked()
{
  int nextIndex = this->Ui->swTracks->currentIndex() + 1;
  if (nextIndex < this->Ui->swTracks->count())
    {
    this->Ui->swTracks->setCurrentIndex(nextIndex);
    if (nextIndex == this->Ui->swTracks->count() - 1) // last track
      {
      this->Ui->pbNext->setEnabled(false);
      }

    if (!this->Ui->pbPrevious->isEnabled())
      {
      this->Ui->pbPrevious->setEnabled(true);
      }
    }
}

// ----------------------------------------------------------------------------
void pqCinemaTrackSelection::populateTracks(QList<pqPipelineFilter*> tracks)
{
  Qt::WindowFlags parentFlags = this->Ui->swTracks->windowFlags();
  foreach(pqPipelineFilter* t, tracks)
    {
    // add only cinema-supported filters
    if (t->getProxy()->GetVTKClassName() &&
       (!strcmp(t->getProxy()->GetVTKClassName(), "vtkPVContourFilter")    ||
        !strcmp(t->getProxy()->GetVTKClassName(), "vtkPVMetaSliceDataSet") ||
        !strcmp(t->getProxy()->GetVTKClassName(), "vtkPVMetaClipDataSet")) )
      {
      pqCinemaTrack* track = new pqCinemaTrack(this->Ui->swTracks, parentFlags, t);
      this->Ui->swTracks->addWidget(track);
      }
    }

  if (this->Ui->swTracks->count() > 1)
    {
    this->Ui->swTracks->setCurrentIndex(0);
    this->Ui->pbNext->setEnabled(true);
    }
}

//-----------------------------------------------------------------------------
QList<pqCinemaTrack*> pqCinemaTrackSelection::getTracks()
{
  QList<pqCinemaTrack*> tracks;

  for (int i = 0 ; i < this->Ui->swTracks->count() ; i++)
    {
    if (pqCinemaTrack* track = qobject_cast<pqCinemaTrack*>(
          this->Ui->swTracks->widget(i)) )
      {
      tracks.append(track);
      }
    }

  return tracks;
}

//------------------------------------------------------------------------------
QString pqCinemaTrackSelection::getSelectionAsPythonScript(QString const & format)
{
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

    if (index < tracks.count() - 1)
      cinema_tracks += ", ";
    }

  return cinema_tracks;
}

// ----------------------------------------------------------------------------
//void pqCinemaTrackSelection::populateArrayPicker(vtkSMSourceProxy* proxy)
//{
//  // clear and re-populate value-arrays
//  pqTreeWidget* arrayPicker = this->Ui->wArrayPicker;
//  arrayPicker->clear();
//
//  vtkPVDataInformation* dataInfo = proxy->GetDataInformation();
//  if (!dataInfo)
//    {
//    return;
//    }
//  
//  vtkPVDataSetAttributesInformation const* attribInfo = dataInfo->GetPointDataInformation();
//  if (!attribInfo)
//    {
//    return;
//    }
//
//  int const numArrays = attribInfo->GetNumberOfArrays();
//  for (int i = 0 ; i < numArrays ; i++)
//    {
//      vtkPVArrayInformation* arrInfo = attribInfo->GetArrayInformation(i);
//      if (!arrInfo)
//        {
//        continue;
//        }
//
//      //QTreeWidgetItem* item = new QTreeWidgetItem(arrayPicker);
//      pqTreeWidgetItem* item = new pqTreeWidgetItem(arrayPicker);
//      item->setData(0, Qt::DisplayRole, arrInfo->GetName());
//      //item->setData(0, Qt::DecorationRole, pixmaps[k]);
//      QString dataType = vtkImageScalarTypeNameMacro(arrInfo->GetDataType());
//      item->setData(1, Qt::DisplayRole, dataType);
//      //item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable); // Qt::ItemIsSelectable
//
//      arrayPicker->header()->resizeSections(QHeaderView::ResizeToContents);
//      //arrayPicker->setItemDelegate(new pqNonEditableStyledItemDelegate(this));
//      //qDebug() << "->>> Array " << i << ": " << QString(arrInfo->GetName());
//    }
//}
