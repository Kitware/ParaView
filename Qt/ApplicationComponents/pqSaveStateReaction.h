/*=========================================================================

   Program: ParaView
   Module:    pqSaveStateReaction.h

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
#ifndef pqSaveStateReaction_h
#define pqSaveStateReaction_h

#include "pqReaction.h"

/**
* @ingroup Reactions
* Reaction for saving state file.
*/
class PQAPPLICATIONCOMPONENTS_EXPORT pqSaveStateReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  /**
  * Constructor. Parent cannot be NULL.
  */
  pqSaveStateReaction(QAction* parent);
  ~pqSaveStateReaction() override {}

  /**
  * Open File dialog in order to choose the location and the type of
  * the state file that should be saved
  * Returns true if the user selected a file to save and false if they canceled the dialog
  */
  static bool saveState();

  /**
  * Saves the state file.
  * Note that this method is static. Applications can simply use this without
  * having to create a reaction instance.
  * Return true if the operation succeeded otherwise return false.
  */
  static bool saveState(const QString& filename);

  /**
  * Saves the state file as a python state.
  * Note that this method is static. Applications can simply use this without
  * having to create a reaction instance.
  * Return true if the operation succeeded otherwise return false.
  */
  static bool savePythonState(const QString& filename);

public Q_SLOTS:
  /**
  * Updates the enabled state. Applications need not explicitly call
  * this.
  */
  void updateEnableState() override;

protected:
  /**
  * Called when the action is triggered.
  */
  void onTriggered() override { pqSaveStateReaction::saveState(); }

private:
  Q_DISABLE_COPY(pqSaveStateReaction)
};

#endif
