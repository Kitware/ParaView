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
#include "pqPipelineFilter.h"
#include "pqCinemaTrack.h"
#include "pqApplicationCore.h"
#include "pqServerManagerModel.h"
#include "pqOutputPort.h"
#include "pqPipelineModel.h"
#include "pqRenderView.h"
#include "pqTreeWidget.h"


// ----------------------------------------------------------------------------
pqCinemaTrackSelection::pqCinemaTrackSelection(QWidget* parent_)
: QWidget(parent_)
, Ui(new Ui::CinemaTrackSelection())
{
  this->Ui->setupUi(this);
  this->initializePipelineBrowser();

  connect(this->Ui->pbPrevious, SIGNAL(clicked()), this, SLOT(onPreviousClicked()));
  connect(this->Ui->pbNext, SIGNAL(clicked()), this, SLOT(onNextClicked()));
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
  pqPipelineModel* model = new pqPipelineModel(*smModel, NULL);

  // pqPipelineBrowser takes ownership of the model
  pqPipelineBrowserWidget* plBrowser = this->Ui->wPipelineBrowser;
  plBrowser->setModel(model);
  plBrowser->expandAll();

  QItemSelectionModel* selModel = plBrowser->getSelectionModel();
  connect(selModel, SIGNAL(currentChanged(QModelIndex const &, QModelIndex const &)),
    this, SLOT(onPipelineItemChanged(QModelIndex const &, QModelIndex const &)));

  // TODO change the view depending on the wViewSelection FIXME
  QList<pqRenderViewBase*> views = smModel->findItems<pqRenderViewBase*>();
  plBrowser->setActiveView(views[0]);
}

// ----------------------------------------------------------------------------
void pqCinemaTrackSelection::onPipelineItemChanged(QModelIndex const & current,
  QModelIndex const & previous)
{
  // transform the FilterModel QMIndex to its PipelineModel equivalent
  pqPipelineModel const* model = this->Ui->wPipelineBrowser->getPipelineModel(current);
  QModelIndex const plModelIndex = this->Ui->wPipelineBrowser->pipelineModelIndex(current);
  // query an item from the PipelineModel and get its vtkSMSourceProxy
  pqServerManagerModelItem const* smModelItem = model->getItemFor(plModelIndex);
  pqPipelineSource const* source = qobject_cast<pqPipelineSource const*>(smModelItem);
  pqOutputPort const* port = source ? source->getOutputPort(0) :
    qobject_cast<pqOutputPort const*>(smModelItem);

  if (!port)
    {
    qDebug() << "Failed to query outputPort from index(" << current.row() << ", "
      << current.column() << ")";
    return;
    }

  vtkSMSourceProxy* proxy = port->getSourceProxy();
  //proxy->PrintSelf(std::cout, vtkIndent());
  this->populateArrayPicker(proxy);
}

// ----------------------------------------------------------------------------
void pqCinemaTrackSelection::populateArrayPicker(vtkSMSourceProxy* proxy)
{
  // clear and re-populate value-arrays
  pqTreeWidget* arrayPicker = this->Ui->wArrayPicker;
  arrayPicker->clear();

  vtkPVDataInformation* dataInfo = proxy->GetDataInformation();
  if (!dataInfo)
    {
    return;
    }
  
  vtkPVDataSetAttributesInformation const* attribInfo = dataInfo->GetPointDataInformation();
  if (!attribInfo)
    {
    return;
    }

  for (int i = 0 ; i < attribInfo->GetNumberOfArrays() ; i++)
    {
      vtkPVArrayInformation* arrInfo = attribInfo->GetArrayInformation(i);
      if (!arrInfo)
        {
        continue;
        }

      qDebug() << "->>> Array " << i << ": " << QString(arrInfo->GetName());
    }
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
