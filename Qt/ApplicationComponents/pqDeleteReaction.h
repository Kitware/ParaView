/*=========================================================================

   Program: ParaView
   Module:    pqDeleteReaction.h

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
#ifndef pqDeleteReaction_h
#define pqDeleteReaction_h

#include "pqReaction.h"

class pqPipelineSource;
class pqProxy;

/**
* @ingroup Reactions
* Reaction for delete sources (all or selected only).
*/
class PQAPPLICATIONCOMPONENTS_EXPORT pqDeleteReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  /**
  * if delete_all is false, then only selected items will be deleted if
  * possible.
  */
  pqDeleteReaction(QAction* parent, bool delete_all = false);

  static void deleteAll();
  static void deleteSelected();
  static bool canDeleteSelected();

  /**
  * Deletes all sources in the set, if possible.
  * All variants of public methods on this class basically call this method
  * with the sources set built up appropriately.
  * The sources set is
  * modified to remove all deleted sources. Any undeleted sources will remain
  * in the set.
  */
  static void deleteSources(const QSet<pqProxy*>& sources);

public Q_SLOTS:
  /**
  * Updates the enabled state. Applications need not explicitly call
  * this.
  */
  void updateEnableState() override;

  /**
  * Request deletion of a particular source.
  */
  void deleteSource(pqProxy* source);

protected:
  /**
  * Called when the action is triggered.
  */
  void onTriggered() override;

private:
  Q_DISABLE_COPY(pqDeleteReaction)
  bool DeleteAll;

  /**
  * Method called just before deleting a source.
  * Updates to the UI before deletion are done here.
  */
  static void aboutToDelete(pqProxy* source);
};

#endif
