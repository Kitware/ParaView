/*=========================================================================

   Program: ParaView
   Module:  pqEditScalarBarReaction.h

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
#ifndef pqEditScalarBarReaction_h
#define pqEditScalarBarReaction_h

#include "pqReaction.h"
#include <QPointer>

class pqDataRepresentation;
class pqScalarBarVisibilityReaction;

/**
* @ingroup Reactions
* Reaction to allow editing of scalar bar properties using a
* pqProxyWidgetDialog.
*
* Reaction allows editing of scalar bar properties using a
* pqProxyWidgetDialog. Internally, it uses pqScalarBarVisibilityReaction to
* track the visibility state for the scalar to enable/disable the parent
* action.
*/
class PQAPPLICATIONCOMPONENTS_EXPORT pqEditScalarBarReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  pqEditScalarBarReaction(QAction* parent = 0, bool track_active_objects = true);
  ~pqEditScalarBarReaction() override;

public Q_SLOTS:
  /**
  * Set the active representation. This should only be used when
  * \c track_active_objects is false. If used when \c track_active_objects is
  * true, the representation will get replaced whenever the active
  * representation changes.
  */
  void setRepresentation(pqDataRepresentation*);

  /**
  * Show the editor dialog for editing scalar bar properties.
  */
  bool editScalarBar();

protected Q_SLOTS:
  /**
  * Updates the enabled state. Applications need not explicitly call
  * this.
  */
  void updateEnableState() override;

  /**
  * Called when the action is triggered.
  */
  void onTriggered() override;

private:
  Q_DISABLE_COPY(pqEditScalarBarReaction)
  QPointer<pqScalarBarVisibilityReaction> SBVReaction;
};

#endif
