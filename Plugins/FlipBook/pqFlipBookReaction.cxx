// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqFlipBookReaction.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqDataRepresentation.h"
#include "pqKeySequences.h"
#include "pqModalShortcut.h"
#include "pqPipelineModel.h"
#include "pqRenderView.h"
#include "pqServerManagerModel.h"

#include "vtkCollection.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"

#include <QKeySequence>

//-----------------------------------------------------------------------------
pqFlipBookReaction::pqFlipBookReaction(
  QAction* toggleAction, QAction* playAction, QAction* stepAction, QSpinBox* playDelay)
  : Superclass(toggleAction)
  , PlayAction(playAction)
  , StepAction(stepAction)
  , PlayDelay(playDelay)
{
  this->StepActionMode = pqKeySequences::instance().addModalShortcut(
    QKeySequence(Qt::Key_Space), this->StepAction, toggleAction->parentWidget());

  QObject::connect(toggleAction, SIGNAL(toggled(bool)), this, SLOT(onToggled(bool)));
  QObject::connect(playAction, SIGNAL(triggered()), this, SLOT(onPlay()));
  QObject::connect(stepAction, SIGNAL(triggered(bool)), this, SLOT(onStepClicked()));

  this->Timer = new QTimer(this);
  QObject::connect(this->Timer, SIGNAL(timeout()), this, SLOT(updateVisibility()));

  QObject::connect(&pqActiveObjects::instance(), SIGNAL(viewChanged(pqView*)), this,
    SLOT(updateEnableState()), Qt::QueuedConnection);

  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  QObject::connect(smmodel, SIGNAL(viewAdded(pqView*)), this, SLOT(updateEnableState()));
  QObject::connect(smmodel, SIGNAL(viewRemoved(pqView*)), this, SLOT(updateEnableState()));
  QObject::connect(
    smmodel, SIGNAL(sourceAdded(pqPipelineSource*)), this, SLOT(updateEnableState()));
  QObject::connect(
    smmodel, SIGNAL(sourceRemoved(pqPipelineSource*)), this, SLOT(updateEnableState()));

  this->updateEnableState();
}

//-----------------------------------------------------------------------------
void pqFlipBookReaction::updateEnableState()
{
  pqRenderView* view = qobject_cast<pqRenderView*>(pqActiveObjects::instance().activeView());

  if (view && view != this->View)
  {
    this->View = view;
    QObject::connect(this->View, SIGNAL(representationAdded(pqRepresentation*)), this,
      SLOT(representationsModified(pqRepresentation*)));
    QObject::connect(this->View, SIGNAL(representationRemoved(pqRepresentation*)), this,
      SLOT(representationsModified(pqRepresentation*)));
    QObject::connect(this->View, SIGNAL(representationVisibilityChanged(pqRepresentation*, bool)),
      this, SLOT(representationVisibilityChanged(pqRepresentation*, bool)));

    this->parentAction()->setChecked(false);
  }

  // We must have at least 2 data representations to enable the feature.
  this->parentAction()->setEnabled(this->hasEnoughVisibleRepresentations());

  bool enabled = this->parentAction()->isChecked();
  this->PlayAction->setEnabled(enabled);
  this->StepAction->setEnabled(enabled);
  this->PlayDelay->setEnabled(enabled);
}

//-----------------------------------------------------------------------------
bool pqFlipBookReaction::hasEnoughVisibleRepresentations()
{
  return this->getNumberOfVisibleRepresentations() >= 2;
}

//-----------------------------------------------------------------------------
int pqFlipBookReaction::getNumberOfVisibleRepresentations()
{
  if (!this->View)
  {
    return 0;
  }
  QList<pqRepresentation*> reprs = this->View->getRepresentations();
  int count = 0;
  for (auto* r : reprs)
  {
    count += (r && r->isVisible()) ? 1 : 0;
  }
  return count;
}

//-----------------------------------------------------------------------------
void pqFlipBookReaction::parseVisibleRepresentations()
{
  this->VisibleRepresentations.clear();
  // We fetch source representations through pqPipelineModel because
  // it is the only way to retrieve the sources in the order they appear
  // in the pipeline browser.
  pqServerManagerModel* smModel = pqApplicationCore::instance()->getServerManagerModel();
  pqPipelineModel* pipelineModel = new pqPipelineModel(*smModel, this);
  this->parseVisibleRepresentations(pipelineModel, pipelineModel->index(0, 0));
}

//-----------------------------------------------------------------------------
void pqFlipBookReaction::parseVisibleRepresentations(pqPipelineModel* model, QModelIndex parent)
{
  if (!this->View)
  {
    return;
  }
  for (int i = 0; i < model->rowCount(parent); i++)
  {
    QModelIndex index = model->index(i, 0, parent);
    pqServerManagerModelItem* smModelItem = model->getItemFor(index);
    if (pqPipelineSource* input = qobject_cast<pqPipelineSource*>(smModelItem))
    {
      for (pqDataRepresentation* r : input->getRepresentations(this->View))
      {
        if (r && r->isVisible())
        {
          this->VisibleRepresentations.push_back(r);
        }
      }
    }
    if (model->hasChildren(index))
    {
      this->parseVisibleRepresentations(model, index);
    }
  }
}

//-----------------------------------------------------------------------------
void pqFlipBookReaction::onPlay()
{
  this->onPlay(!this->Timer->isActive());
}

//-----------------------------------------------------------------------------
void pqFlipBookReaction::onPlay(bool play)
{
  if (play)
  {
    this->Timer->start(this->PlayDelay->value());
    this->PlayAction->setIcon(QIcon(":/pqFlipBook/Icons/pqFlipBookPause.png"));
  }
  else
  {
    this->Timer->stop();
    this->PlayAction->setIcon(QIcon(":/pqFlipBook/Icons/pqFlipBookPlay.png"));
  }
}

//-----------------------------------------------------------------------------
void pqFlipBookReaction::onStepClicked()
{
  this->updateVisibility();
}

//-----------------------------------------------------------------------------
void pqFlipBookReaction::onToggled(bool checked)
{
  this->updateEnableState();
  if (checked)
  {
    // disable feature if there are less than 2 visible representations
    if (!this->hasEnoughVisibleRepresentations())
    {
      this->VisibleRepresentations.clear();
      return;
    }
    this->VisibilityIndex = 0;
    this->parseVisibleRepresentations();

    this->StepActionMode->setEnabled(true);
  }
  else
  {
    this->onPlay(false);

    // restore visibility
    for (pqDataRepresentation* r : this->VisibleRepresentations)
    {
      if (this->View->hasRepresentation(r))
      {
        r->setVisible(true);
      }
    }
    this->VisibleRepresentations.clear();
    this->View->render();

    this->StepActionMode->setEnabled(false);
  }
  this->updateEnableState();
}

//-----------------------------------------------------------------------------
void pqFlipBookReaction::representationsModified(pqRepresentation* r)
{
  if (this->parentAction()->isChecked())
  {
    if (qobject_cast<pqDataRepresentation*>(r))
    {
      // data have changed, we need to disable the iterative visibility
      this->parentAction()->setChecked(false);
    }
  }

  this->updateEnableState();
}

//-----------------------------------------------------------------------------
void pqFlipBookReaction::representationVisibilityChanged(pqRepresentation*, bool)
{
  if (!this->VisibleRepresentations.empty())
  {
    return;
  }
  if (!this->hasEnoughVisibleRepresentations())
  {
    this->parentAction()->setEnabled(false);
    this->parentAction()->setChecked(false);
    this->VisibleRepresentations.clear();
  }
  else
  {
    this->parentAction()->setEnabled(true);
  }
}

//-----------------------------------------------------------------------------
void pqFlipBookReaction::updateVisibility()
{
  if (!this->parentAction()->isChecked())
  {
    return;
  }
  int nbReprs = this->VisibleRepresentations.size();
  for (int i = 0; i < nbReprs; i++)
  {
    this->VisibleRepresentations[i]->setVisible(i == this->VisibilityIndex);
  }
  this->VisibilityIndex = (this->VisibilityIndex + 1) % nbReprs;
  this->View->render();
}
