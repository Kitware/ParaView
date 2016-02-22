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
#include "vtkSMProxy.h"

#include "pqCinemaTrackSelection.h"
#include "ui_pqCinemaTrackSelection.h"
#include "pqPipelineFilter.h"
#include "pqCinemaTrack.h"


// ----------------------------------------------------------------------------
pqCinemaTrackSelection::pqCinemaTrackSelection(QWidget* parent_)
: QWidget(parent_)
, Ui(new Ui::CinemaTrackSelection())
{
  this->Ui->setupUi(this);

  connect(this->Ui->pbPrevious, SIGNAL(clicked()), this, SLOT(onPreviousClicked()));
  connect(this->Ui->pbNext, SIGNAL(clicked()), this, SLOT(onNextClicked()));
}

pqCinemaTrackSelection::~pqCinemaTrackSelection()
{
  delete Ui;
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
