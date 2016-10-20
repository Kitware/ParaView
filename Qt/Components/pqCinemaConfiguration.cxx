/*=========================================================================

   Program: ParaView
   Module:  pqCinemaConfiguration.cxx

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
#include "pqCinemaConfiguration.h"
#include "pqApplicationCore.h"
#include "pqContextView.h"
#include "pqPipelineFilter.h"
#include "pqRenderViewBase.h"
#include "pqServerManagerModel.h"
#include "ui_pqCinemaConfiguration.h"

// ----------------------------------------------------------------------------
pqCinemaConfiguration::pqCinemaConfiguration(
  vtkSMProxy* proxy_, vtkSMPropertyGroup* smpgroup, QWidget* parent_)
  : Superclass(proxy_, parent_)
  , Ui(new Ui::CinemaConfiguration())
{
  Q_UNUSED(smpgroup);
  this->Ui->setupUi(this);

  /* This is necessary to display all the widgets correctly given that
     QWidget::adjustSize() (see pqExportReaction.cxx) is constrained to a
     max. of 2/3 of the screen's height
     (http://doc.qt.io/qt-4.8/qwidget.html#adjustSize). */
  this->setMinimumHeight(700);

  // link ui to proxy properties
  this->addPropertyLink(
    this, "viewSelection", SIGNAL(viewSelectionChanged()), proxy_->GetProperty("ViewSelection"));

  this->addPropertyLink(
    this, "trackSelection", SIGNAL(trackSelectionChanged()), proxy_->GetProperty("TrackSelection"));

  this->addPropertyLink(
    this, "arraySelection", SIGNAL(arraySelectionChanged()), proxy_->GetProperty("ArraySelection"));

  // other ui connections
  QObject::connect(this->Ui->wViewSelection, SIGNAL(arraySelectionEnabledChanged(bool)),
    this->Ui->wTrackSelection, SLOT(enableArraySelection(bool)));

  // update ui with current views and filters and connect signals
  this->populateElements();
  this->Ui->wViewSelection->setCinemaVisible(true);
  this->Ui->wViewSelection->setCatalystOptionsVisible(false);
}

// ----------------------------------------------------------------------------
pqCinemaConfiguration::~pqCinemaConfiguration()
{
  delete Ui;
}

// ----------------------------------------------------------------------------
void pqCinemaConfiguration::updateWidget(bool showing_advanced_properties)
{
  Superclass::updateWidget(showing_advanced_properties);
}

// ----------------------------------------------------------------------------
QString pqCinemaConfiguration::viewSelection()
{
  // Parameter format pv_introspect.export_scene expects for each view.
  // (see pv_introspect.py and pqExportViewSelection for more details.
  QString format("'%1' : ['%2', %3, %4, %5, %6, %7, %8]");
  QString script = this->Ui->wViewSelection->getSelectionAsString(format);

  return script;
}

// ----------------------------------------------------------------------------
QString pqCinemaConfiguration::trackSelection()
{
  // Parameter format pv_introspect.export_scene expects for each cinema track.
  // (see pv_introspect.py and pqCinemaTrackSelection for more details.
  QString format("'%1' : %2");
  QString script = this->Ui->wTrackSelection->getTrackSelectionAsString(format);

  return script;
}

// ----------------------------------------------------------------------------
QString pqCinemaConfiguration::arraySelection()
{
  // Parameter format pv_introspect.export_scene expects for user selected arrays.
  // (see pv_introspect.py and pqCinemaTrackSelection for more details.
  QString format("'%1' : %2");
  QString script = this->Ui->wTrackSelection->getArraySelectionAsString(format);

  return script;
}

// ----------------------------------------------------------------------------
void pqCinemaConfiguration::populateElements()
{
  pqServerManagerModel* smModel = pqApplicationCore::instance()->getServerManagerModel();

  QList<pqRenderViewBase*> rViews = smModel->findItems<pqRenderViewBase*>();
  QList<pqContextView*> cViews = smModel->findItems<pqContextView*>();
  this->Ui->wViewSelection->populateViews(rViews, cViews);

  this->Ui->wTrackSelection->initializePipelineBrowser();
}

// ----------------------------------------------------------------------------
void pqCinemaConfiguration::hideEvent(QHideEvent* event_)
{
  emit changeFinished();
  Superclass::hideEvent(event_);
}
