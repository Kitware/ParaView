#include <iostream>

#include "pqAdaptiveControls.h"

#include <QApplication>
#include <QStyle>
#include <QCheckBox>
#include <QPushButton>
#include <QComboBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSpinBox>
#include <QLabel>

#include "pqApplicationCore.h"
#include "pqServerManagerSelectionModel.h"
#include "pqServerManagerModelItem.h"
#include "pqServerManagerModel.h"
#include "pqAdaptiveRenderView.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqActiveView.h"
#include "pqServer.h"
#include "pqDataRepresentation.h"
#include "pqPropertyLinks.h"
#include "vtkSMProperty.h"
#include "vtkSMAdaptiveViewProxy.h"
#include "vtkSMAdaptiveRepresentation.h"

class pqAdaptiveControls::pqInternals
{
public:
  pqPropertyLinks Links;
};


pqAdaptiveControls::pqAdaptiveControls(QWidget* p)
  : QDockWidget("Refinement Inspector", p)
{
  this->Internals = new pqInternals();

  this->currentView = NULL;
  this->currentRep = NULL;
  this->setEnabled(false);

  QWidget *canvas = new QWidget();
  QVBoxLayout *vLayout = new QVBoxLayout();
  canvas->setLayout(vLayout);
  this->setWidget(canvas);
  QHBoxLayout *hl;


  ////////////
  //keep self up to date whenever a new source becomes the active one
  pqServerManagerSelectionModel *selection =
      pqApplicationCore::instance()->getSelectionModel();
  QObject::connect(selection, 
                   SIGNAL(currentChanged(pqServerManagerModelItem*)), 
                   this, SLOT(updateTrackee())
                   );
  pqServerManagerModel * model =
      pqApplicationCore::instance()->getServerManagerModel();
  QObject::connect(model,
                   SIGNAL(sourceAdded(pqPipelineSource*)),
                   this, SLOT(updateTrackee()));
  QObject::connect(model,
                   SIGNAL(sourceRemoved(pqPipelineSource*)),
                   this, SLOT(updateTrackee()));

  ////////////
  QLabel *label;
  label = new QLabel("Per Object controls");
  label->setAlignment(Qt::AlignHCenter);
  vLayout->addWidget(label);

  this->restartButton = new QPushButton("Restart", canvas);
  vLayout->addWidget(this->restartButton);
  this->restartButton->setToolTip("Clears the active filter's cached results and restarts its refinement at lowest resolution.");
  QObject::connect(this->restartButton, SIGNAL(pressed()), this, SLOT(onRestart()));


  this->lockButton = new QCheckBox("Lock Refinement", canvas);
  vLayout->addWidget(this->lockButton);
  this->lockButton->setToolTip("When checked this prevents the active filter from being refined or coarsened.");
                                         
  this->boundsButton = new QCheckBox("Show Piece Bounds", canvas);
  vLayout->addWidget(this->boundsButton);
  this->boundsButton->setToolTip("When checked this turns on diagnostic piece bounding box display for the active filter.");

  hl = new QHBoxLayout();
  hl->addWidget(new QLabel("Depth cut off"));
  this->maxDepthSpinner = new QSpinBox(canvas);
  this->maxDepthSpinner->setMinimum(-1);
  this->maxDepthSpinner->setValue(-1);
  this->maxDepthSpinner->setToolTip("Sets an upper bound on the resolution that the active filter is allowed to refine to.");
  hl->addWidget(this->maxDepthSpinner);
  vLayout->addLayout(hl);
  QObject::connect(this->maxDepthSpinner, SIGNAL(valueChanged(int)), this, SLOT(onSetMaxDepth(int)));

  ////////////
  label = new QLabel("Global controls");
  label->setAlignment(Qt::AlignHCenter);
  vLayout->addWidget(label);

  this->interruptButton = new QPushButton("INTERRUPT", canvas);
  vLayout->addWidget(this->interruptButton);
  this->interruptButton->setToolTip("Tell multipass rendering to stop and immediately show results.");
  QObject::connect(this->interruptButton, SIGNAL(pressed()), this, SLOT(onInterrupt()));

  ////////////
  hl = new QHBoxLayout();
  vLayout->addLayout(hl);

  this->refinementModeButton = new QComboBox(canvas);
  hl->addWidget(this->refinementModeButton);
  this->refinementModeButton->addItem("MANUAL");
  this->refinementModeButton->addItem("AUTOMATIC");
  this->refinementModeButton->setToolTip("Controls whether refinement progresses to full resolution automatically or whether user drives it.");
  QObject::connect(this->refinementModeButton, SIGNAL(activated(int)), 
                   this, SLOT(onRefinementMode(int)));

  this->coarsenButton = new QPushButton("-", canvas);
  hl->addWidget(this->coarsenButton);
  QObject::connect(this->coarsenButton, SIGNAL(pressed()), 
                   this, SLOT(onCoarsen()));
  this->coarsenButton->setToolTip("Displays all (unlocked and) visible filters in lesser detail.");

  this->refineButton = new QPushButton("+", canvas);
  hl->addWidget(this->refineButton);
  this->refineButton->setToolTip("Displays all (unlocked and) visible filters in greater detail.");
  QObject::connect(this->refineButton, SIGNAL(pressed()), 
                   this, SLOT(onRefine()));

}

pqAdaptiveControls::~pqAdaptiveControls()
{
  this->Internals->Links.removeAllPropertyLinks();
  delete this->Internals;
}

void pqAdaptiveControls::updateTrackee()
{
  //break stale connections between widgets and properties
  this->Internals->Links.removeAllPropertyLinks();

  //set to a default state
  this->currentRep = NULL;
  this->currentView = NULL;

  this->boundsButton->setEnabled(false);
  this->lockButton->setEnabled(false);
  this->restartButton->setEnabled(false);
  this->maxDepthSpinner->setEnabled(false);

  //find the active adaptive view
  pqView* view = pqActiveView::instance().current();
  pqAdaptiveRenderView* adaptiveView = 
    qobject_cast<pqAdaptiveRenderView*>(view);
  if (!adaptiveView)
    {
    this->setEnabled(false);
    return;
    }
  this->currentView = adaptiveView;
  this->setEnabled(true);

  //find the active filter
  pqServerManagerModelItem *item =
    pqApplicationCore::instance()->getSelectionModel()->currentItem();
  if (item)
    {
    pqOutputPort* opPort = qobject_cast<pqOutputPort*>(item);
    pqPipelineSource *source = opPort? opPort->getSource() : 
      qobject_cast<pqPipelineSource*>(item);
    if (source)
      {
      pqDataRepresentation* adaptiveRep = 
        source->getRepresentation(0, this->currentView);
      if (adaptiveRep)
        {
        //we have a representation that we can connect to. do so now
        this->connectToRepresentation(source, adaptiveRep);
        }
      else
        {
        //try later when there might be something to connect to
        QObject::connect(source, 
                         SIGNAL(visibilityChanged(pqPipelineSource*, 
                                                  pqDataRepresentation*)),
                         this, 
                         SLOT(connectToRepresentation(pqPipelineSource*,
                                                      pqDataRepresentation*))
                         );
        }
      }
    }
}

//---------------------------------------------------------------------------
void pqAdaptiveControls::connectToRepresentation(
  pqPipelineSource* source,
  pqDataRepresentation* rep)
{
  pqDataRepresentation* adaptiveRep = 
    source->getRepresentation(0, this->currentView);
  if (!adaptiveRep)
    {
    return;
    }
  //stop watching what we were
  this->Internals->Links.removeAllPropertyLinks();
  QObject::disconnect(source, SIGNAL(visibilityChanged(pqPipelineSource*,
                                              pqDataRepresentation*)),
                     this,
                     SLOT(connectToRepresentation(pqPipelineSource*,
                                                  pqDataRepresentation*))
                     );

  this->currentRep = adaptiveRep;  

  //establish links to to its properties
  vtkSMProxy *displayProxy = this->currentRep->getProxy();
  this->Internals->Links.addPropertyLink(
    this->boundsButton, "checked", SIGNAL(toggled(bool)),
    displayProxy, 
    displayProxy->GetProperty("PieceBoundsVisibility"));
  
  this->Internals->Links.addPropertyLink(
    this->lockButton, "checked", SIGNAL(toggled(bool)),
    displayProxy, displayProxy->GetProperty("Locked"));

  int maxD;
  vtkSMAdaptiveRepresentation *adaptProxy = 
    vtkSMAdaptiveRepresentation::SafeDownCast(displayProxy);
  maxD = adaptProxy->GetMaxDepth();
  this->maxDepthSpinner->setValue(maxD);
  
  this->boundsButton->setEnabled(true);
  this->lockButton->setEnabled(true);
  this->restartButton->setEnabled(true);
  this->maxDepthSpinner->setEnabled(true);

}

//------------------------------------------------------------------------------
void pqAdaptiveControls::onInterrupt()
{
  if (!this->currentView)
    {
    return;
    }
  this->currentView->getAdaptiveViewProxy()->Interrupt();
  this->onRefinementMode(0);//MANUAL
}

//------------------------------------------------------------------------------
void pqAdaptiveControls::onRestart()
{
  if (!this->currentView || !this->currentRep)
    {
    return;
    }
  vtkSMProxy *displayProxy = this->currentRep->getProxy();
  displayProxy->InvokeCommand("ClearStreamCache");
  this->refinementModeButton->setCurrentIndex(0); //MANUAL
  this->onRefinementMode(0);
}

//------------------------------------------------------------------------------
void pqAdaptiveControls::onSetMaxDepth(int value)
{
  if (!this->currentView || !this->currentRep)
    {
    return;
    }
  vtkSMAdaptiveRepresentation *displayProxy = 
    vtkSMAdaptiveRepresentation::SafeDownCast(this->currentRep->getProxy());
  displayProxy->SetMaxDepth(value);
}

//------------------------------------------------------------------------------
void pqAdaptiveControls::onRefinementMode(int index)
{
  if (!this->currentView)
    {
    return;
    }
  this->currentView->getAdaptiveViewProxy()->SetRefinementMode(index);
  if (index==1)
    {
    this->coarsenButton->setEnabled(false);
    this->refineButton->setEnabled(false);
    }
  else
    {
    this->coarsenButton->setEnabled(true);
    this->refineButton->setEnabled(true);
    }
  this->currentView->render();
}

//------------------------------------------------------------------------------
void pqAdaptiveControls::onCoarsen()
{
  if (!this->currentView)
    {
    return;
    }
  this->currentView->getAdaptiveViewProxy()->Coarsen();
  this->currentView->render();
}

//------------------------------------------------------------------------------
void pqAdaptiveControls::onRefine()
{
  if (!this->currentView)
    {
    return;
    }
  this->currentView->getAdaptiveViewProxy()->Refine();
  this->currentView->render();
}

