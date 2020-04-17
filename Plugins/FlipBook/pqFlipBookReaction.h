/*=========================================================================

   Program: ParaView
   Module:    pqFlipBookReaction.h

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
#ifndef pqFlipBookReaction_h
#define pqFlipBookReaction_h

#include "pqReaction.h"

#include <QPointer>
#include <QSpinBox>

class pqDataRepresentation;
class pqPipelineModel;
class pqRepresentation;
class pqView;

class QShortcut;

/**
 * @ingroup Reactions
 * pqFlipBookReaction is a reaction to iterative visibility button.
 */
class pqFlipBookReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  pqFlipBookReaction(QAction* parent, QAction* playAction, QAction* stepAction, QSpinBox* autoVal);
  ~pqFlipBookReaction() override = default;

public Q_SLOTS:
  /**
   * Updates the enabled state. Applications need not explicitly call this.
   */
  void updateEnableState() override;

protected Q_SLOTS:
  /**
   * Called when the action is toggled.
   */
  void onToggled(bool checked);

  /**
   * Called when the play action is toggled.
   */
  void onPlay();

  /**
   * Called when the step action is clicked.
   */
  void onStepClicked();

  /**
   * Update visibility of data representations based on current index
   */
  void updateVisibility();

  /**
   * Triggered when a data representation is added or removed
   */
  void representationsModified(pqRepresentation*);

  void representationVisibilityChanged(pqRepresentation*, bool);

protected:
  bool hasEnoughVisibleRepresentations();

  int getNumberOfVisibleRepresentations();

  void parseVisibleRepresentations();
  void parseVisibleRepresentations(pqPipelineModel*, QModelIndex);

  void onPlay(bool play);

private:
  Q_DISABLE_COPY(pqFlipBookReaction);

  QPointer<QAction> PlayAction;
  QPointer<QAction> StepAction;
  QPointer<QSpinBox> PlayDelay;

  QPointer<pqView> View;
  QTimer* Timer;
  QPointer<QShortcut> ShortCutNext;

  QList<QPointer<pqDataRepresentation> > VisibleRepresentations;
  int VisibilityIndex;
};

#endif
