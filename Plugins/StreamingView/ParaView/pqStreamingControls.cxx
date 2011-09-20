/*=========================================================================

  Program:   Visualization Toolkit
  Module:    pqMantaView.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

  Program:   VTK/ParaView Los Alamos National Laboratory Modules (PVLANL)
  Module:    pqMantaView.h

Copyright (c) 2007, Los Alamos National Security, LLC

All rights reserved.

Copyright 2007. Los Alamos National Security, LLC.
This software was produced under U.S. Government contract DE-AC52-06NA25396
for Los Alamos National Laboratory (LANL), which is operated by
Los Alamos National Security, LLC for the U.S. Department of Energy.
The U.S. Government has rights to use, reproduce, and distribute this software.
NEITHER THE GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY, LLC MAKES ANY WARRANTY,
EXPRESS OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE.
If software is modified to produce derivative works, such modified software
should be clearly marked, so as not to confuse it with the version available
from LANL.

Additionally, redistribution and use in source and binary forms, with or
without modification, are permitted provided that the following conditions
are met:
-   Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.
-   Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
-   Neither the name of Los Alamos National Security, LLC, Los Alamos National
    Laboratory, LANL, the U.S. Government, nor the names of its contributors
    may be used to endorse or promote products derived from this software
    without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY LOS ALAMOS NATIONAL SECURITY, LLC AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL LOS ALAMOS NATIONAL SECURITY, LLC OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "pqStreamingControls.h"
#include "ui_pqStreamingControls.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqDataRepresentation.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqPropertyLinks.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqServerManagerModelItem.h"
#include "pqServerManagerSelectionModel.h"
#include "pqSignalAdaptors.h"

#include "IteratingView.h"
#include "PrioritizingView.h"
#include "RefiningView.h"
#include "StreamingView.h"

#include "vtkSMPropertyHelper.h"
#include "vtkSMStreamingViewProxy.h"
#include "vtkSMPVRepresentationProxy.h"

class pqStreamingControls::pqInternals
  : public Ui::pqStreamingControls
{
public:
  pqInternals()
  {
    this->CacheSizeAdaptor = NULL;
  }
  ~pqInternals()
  {
    delete this->CacheSizeAdaptor;
  }

  pqPropertyLinks ViewLinks;
  pqPropertyLinks RepresentationLinks;
  pqSignalAdaptorComboBox* CacheSizeAdaptor;
};

//------------------------------------------------------------------------------
pqStreamingControls::pqStreamingControls(QWidget* p)
  : QDockWidget("Streaming Inspector", p)
{
  this->Internals = new pqInternals();
  this->Internals->setupUi(this);

  //mapping from the cache_size QComboBox to a number of pieces
  this->Internals->cache_size->setItemData(0, -1);
  this->Internals->cache_size->setItemData(1, 0);
  this->Internals->cache_size->setItemData(2, 1);
  this->Internals->cache_size->setItemData(3, 2);
  this->Internals->cache_size->setItemData(4, 4);
  this->Internals->cache_size->setItemData(5, 8);
  this->Internals->cache_size->setItemData(6, 16);
  this->Internals->cache_size->setItemData(7, 32);
  this->Internals->cache_size->setItemData(8, 64);
  this->Internals->cache_size->setItemData(9, 128);
  this->Internals->cache_size->setItemData(10, 256);
  this->Internals->cache_size->setItemData(11, 512);
  this->Internals->cache_size->setItemData(12, 1024);
  this->Internals->cache_size->setItemData(13, 2048);
  this->Internals->cache_size->setItemData(14, 4096);
  this->Internals->cache_size->setItemData(15, 8192);
  this->Internals->cache_size->setItemData(16, 16384);
  this->Internals->cache_size->setItemData(17, 32768);
  this->Internals->cache_size->setItemData(18, 65536);

  this->Internals->CacheSizeAdaptor = new pqSignalAdaptorComboBox(
    this->Internals->cache_size);

  this->currentView = NULL;
  this->currentRep = NULL;
  this->setEnabled(false);

  //keep self up to date whenever the active view changes
  QObject::connect(&pqActiveObjects::instance(),
                   SIGNAL(viewChanged(pqView*)),
                   this, SLOT(updateTrackedView()));

  //or a new source becomes the active one
  QObject::connect(&pqActiveObjects::instance(),
                   SIGNAL(representationChanged(pqDataRepresentation*)),
                   this, SLOT(updateTrackedRepresentation()));

  //connect command widgets to handlers for them
  //state widgets, that are synched to the view and rep are handled by the Links
  QObject::connect(this->Internals->stop, SIGNAL(pressed()),
                   this, SLOT(onStop()));
  QObject::connect(this->Internals->refine, SIGNAL(pressed()),
                   this, SLOT(onRefine()));
  QObject::connect(this->Internals->coarsen, SIGNAL(pressed()),
                   this, SLOT(onCoarsen()));
  QObject::connect(this->Internals->restart_refinement, SIGNAL(pressed()),
                   this, SLOT(onRestartRefinement()));

  //some enabled/disabled links
  QObject::connect(this->Internals->progression_mode,
                   SIGNAL(currentIndexChanged(int)),
                   this, SLOT(onProgressionMode(int)));
}

//------------------------------------------------------------------------------
pqStreamingControls::~pqStreamingControls()
{
  this->Internals->ViewLinks.removeAllPropertyLinks();
  this->Internals->RepresentationLinks.removeAllPropertyLinks();
  delete this->Internals;
}

//------------------------------------------------------------------------------
void pqStreamingControls::updateTrackedView()
{
  //cerr << "CHECK VIEW" << endl;
  //find the active adaptive view
  pqView* view = pqActiveObjects::instance().activeView();
  if (view == this->currentView)
    {
    return;
    }

  //break stale connections between widgets and properties
  this->Internals->ViewLinks.removeAllPropertyLinks();

  StreamingView* sView = qobject_cast<StreamingView*>(view);
  if (!sView)
    {
    this->currentView = NULL;
    this->setEnabled(false);
    return;
    }
  this->currentView = sView;
  this->setEnabled(true);

  vtkSMStreamingViewProxy *svp =
    vtkSMStreamingViewProxy::SafeDownCast(sView->getProxy());
  vtkSMProxy *driver = svp->GetDriver();
  //TODO change to this when driver exists early in the SMSVP
  if (!driver)
    {
    return;
    }

  //change the available controls to match the streaming algorihm
  //used by the view. At the same time, update the widget<->View
  //properties links.
  IteratingView* iView = qobject_cast<IteratingView*>(view);
  PrioritizingView* pView = qobject_cast<PrioritizingView*>(view);
  RefiningView* rView = qobject_cast<RefiningView*>(view);
  if (iView)
    {
    //cerr << "FOUND IVIEW " << iView << endl;
    this->Internals->streaming_controls->setEnabled(true);
    this->Internals->number_of_passes->setEnabled(true);
    this->Internals->last_pass->setEnabled(true);
    this->Internals->prioritization_controls->setEnabled(false);
    this->Internals->refinement_controls->setEnabled(false);

    this->Internals->ViewLinks.addPropertyLink
      (this->Internals->show_when, "currentIndex",
       SIGNAL(currentIndexChanged(int)),
       driver, driver->GetProperty("ShowWhen"));

    this->Internals->ViewLinks.addPropertyLink
      (this->Internals->CacheSizeAdaptor, "currentData",
       SIGNAL(currentIndexChanged(int)),
       driver, driver->GetProperty("CacheSize"));

    this->Internals->ViewLinks.addPropertyLink
      (this->Internals->number_of_passes, "value", SIGNAL(valueChanged(int)),
       driver, driver->GetProperty("NumberOfPasses"));

    this->Internals->ViewLinks.addPropertyLink
      (this->Internals->last_pass, "value", SIGNAL(valueChanged(int)),
       driver, driver->GetProperty("LastPass"));
    }
  else if (pView)
    {
    //cerr << "FOUND PVIEW " << pView << endl;
    this->Internals->streaming_controls->setEnabled(true);
    this->Internals->number_of_passes->setEnabled(true);
    this->Internals->last_pass->setEnabled(true);
    this->Internals->prioritization_controls->setEnabled(true);
    this->Internals->refinement_controls->setEnabled(false);

    this->Internals->ViewLinks.addPropertyLink
      (this->Internals->show_when, "currentIndex",
       SIGNAL(currentIndexChanged(int)),
       driver, driver->GetProperty("ShowWhen"));

    this->Internals->ViewLinks.addPropertyLink
      (this->Internals->CacheSizeAdaptor, "currentData",
       SIGNAL(currentIndexChanged(int)),
       driver, driver->GetProperty("CacheSize"));

    this->Internals->ViewLinks.addPropertyLink
      (this->Internals->number_of_passes, "value", SIGNAL(valueChanged(int)),
       driver, driver->GetProperty("NumberOfPasses"));

    this->Internals->ViewLinks.addPropertyLink
      (this->Internals->last_pass, "value", SIGNAL(valueChanged(int)),
       driver, driver->GetProperty("LastPass"));

    this->Internals->ViewLinks.addPropertyLink
      (this->Internals->pipeline_priority, "checked", SIGNAL(stateChanged(int)),
       driver, driver->GetProperty("PipelinePrioritization"));

    this->Internals->ViewLinks.addPropertyLink
      (this->Internals->view_priority, "checked", SIGNAL(stateChanged(int)),
       driver, driver->GetProperty("ViewPrioritization"));
    }
  else if (rView)
    {
    //cerr << "FOUND RVIEW " << rView << endl;
    this->Internals->streaming_controls->setEnabled(true);
    this->Internals->number_of_passes->setEnabled(false);
    this->Internals->last_pass->setEnabled(false);
    this->Internals->prioritization_controls->setEnabled(true);
    this->Internals->refinement_controls->setEnabled(true);

    this->Internals->ViewLinks.addPropertyLink
      (this->Internals->show_when, "currentIndex",
       SIGNAL(currentIndexChanged(int)),
       driver, driver->GetProperty("ShowWhen"));

   this->Internals->ViewLinks.addPropertyLink
      (this->Internals->CacheSizeAdaptor, "currentData",
       SIGNAL(currentIndexChanged(int)),
       driver, driver->GetProperty("CacheSize"));

    this->Internals->ViewLinks.addPropertyLink
      (this->Internals->pipeline_priority, "checked", SIGNAL(stateChanged(int)),
       driver, driver->GetProperty("PipelinePrioritization"));

    this->Internals->ViewLinks.addPropertyLink
      (this->Internals->view_priority, "checked", SIGNAL(stateChanged(int)),
       driver, driver->GetProperty("ViewPrioritization"));

    this->Internals->ViewLinks.addPropertyLink
      (this->Internals->refinement_depth,
       "currentIndex", SIGNAL(currentIndexChanged(int)),
       driver, driver->GetProperty("RefinementDepth"));

    this->Internals->ViewLinks.addPropertyLink
      (this->Internals->cell_pixel_factor, "value",
       SIGNAL(valueChanged(double)),
       driver, driver->GetProperty("CellPixelFactor"));

    this->Internals->ViewLinks.addPropertyLink
      (this->Internals->back_face_factor, "value",
       SIGNAL(valueChanged(double)),
       driver, driver->GetProperty("BackFaceFactor"));

    this->Internals->ViewLinks.addPropertyLink
      (this->Internals->max_depth, "value", SIGNAL(valueChanged(int)),
       driver, driver->GetProperty("DepthLimit"));

    this->Internals->ViewLinks.addPropertyLink
      (this->Internals->max_splits, "value", SIGNAL(valueChanged(int)),
       driver, driver->GetProperty("MaxSplits"));

    this->Internals->ViewLinks.addPropertyLink
      (this->Internals->progression_mode,
       "currentIndex", SIGNAL(currentIndexChanged(int)),
       driver, driver->GetProperty("ProgressionMode"));
    }
  else
    {
    cerr << "Can not recognize that streaming view type." << endl;
    }

  this->updateTrackedRepresentation();
}

//------------------------------------------------------------------------------
void pqStreamingControls::updateTrackedRepresentation()
{
  //cerr << "CHECK REP" << endl;
  //break stale connections between widgets and properties
  this->Internals->RepresentationLinks.removeAllPropertyLinks();
  this->currentRep = NULL;
  this->Internals->lock_refinement->setCheckState(Qt::Unchecked);

  //find the active filter so that this panel can reflect its properties
  pqDataRepresentation* rep =
    pqActiveObjects::instance().activeRepresentation();
  if (rep)
    {
    vtkSMPVRepresentationProxy *sRepProxy =
      vtkSMPVRepresentationProxy::SafeDownCast(rep->getProxy());
    if (!sRepProxy)
      {
      return;
      }
    this->currentRep = sRepProxy;
    //cerr << "FOUND SREP " << this->currentRep << endl;

    RefiningView* rView = qobject_cast<RefiningView*>(this->currentView);
    if (rView)
      {
      this->Internals->RepresentationLinks.addPropertyLink
        (this->Internals->lock_refinement, "checked", SIGNAL(stateChanged(int)),
         sRepProxy, sRepProxy->GetProperty("LockRefinement"));
      }
    }
}

//------------------------------------------------------------------------------
void pqStreamingControls::onStop()
{
  if (!this->currentView)
    {
    return;
    }
  vtkSMStreamingViewProxy *svp =
    vtkSMStreamingViewProxy::SafeDownCast(this->currentView->getProxy());
  vtkSMProxy *driver = svp->GetDriver();
  driver->InvokeCommand("StopStreaming");
}

//------------------------------------------------------------------------------
void pqStreamingControls::onRefine()
{
  RefiningView* rView = qobject_cast<RefiningView*>(this->currentView);
  if (!rView)
    {
    return;
    }
  vtkSMStreamingViewProxy *svp =
    vtkSMStreamingViewProxy::SafeDownCast(this->currentView->getProxy());
  vtkSMProxy *driver = svp->GetDriver();
  driver->InvokeCommand("Refine");
  rView->render();
}

//------------------------------------------------------------------------------
void pqStreamingControls::onCoarsen()
{
  RefiningView* rView = qobject_cast<RefiningView*>(this->currentView);
  if (!rView)
    {
    return;
    }
  vtkSMStreamingViewProxy *svp =
    vtkSMStreamingViewProxy::SafeDownCast(this->currentView->getProxy());
  vtkSMProxy *driver = svp->GetDriver();
  driver->InvokeCommand("Coarsen");
  rView->render();
}

//------------------------------------------------------------------------------
void pqStreamingControls::onRestartRefinement()
{
  RefiningView* rView = qobject_cast<RefiningView*>(this->currentView);
  if (rView && this->currentRep)
    {
    vtkSMStreamingViewProxy *svp =
      vtkSMStreamingViewProxy::SafeDownCast(this->currentView->getProxy());
    vtkSMProxy *driver = svp->GetDriver();
    driver->InvokeCommand("RestartStreaming");
    this->currentRep->InvokeCommand("RestartRefinement");
    rView->render();
    }
}

//------------------------------------------------------------------------------
void pqStreamingControls::onProgressionMode(int state)
{
  if (state == 0)
    {
    //manual
    this->Internals->refine->setEnabled(true);
    this->Internals->coarsen->setEnabled(true);
    }
  else
    {
    //automatic
    this->Internals->refine->setEnabled(false);
    this->Internals->coarsen->setEnabled(false);
    RefiningView* rView = qobject_cast<RefiningView*>(this->currentView);
    if (rView)
      {
      rView->render();
      }
    }
}
