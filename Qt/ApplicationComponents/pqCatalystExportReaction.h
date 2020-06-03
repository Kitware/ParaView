/*=========================================================================

   Program: ParaView
   Module:    pqCatalystExportReaction.h

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
#ifndef pqCatalystExportReaction_h
#define pqCatalystExportReaction_h

#include "pqApplicationComponentsModule.h"
#include "pqReaction.h"

/**
* @ingroup Reactions
* Reaction to export a Catalyst script that will produce configured catalyst data products.
*/

class PQAPPLICATIONCOMPONENTS_EXPORT pqCatalystExportReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  pqCatalystExportReaction(QAction* parent);
  ~pqCatalystExportReaction();

  /**
   * Export a Catalyst script. Returns true on success.
   */
  static bool exportScript(const QString& name);

  /**
   * Export a Catalyst script. Returns the non-empty name of the file
   * written on success.
   */
  static QString exportScript();

protected Q_SLOTS:
  /**
  * Called when the action is triggered.
  */
  void onTriggered() override { this->exportScript(); }

private:
  Q_DISABLE_COPY(pqCatalystExportReaction)
};

#endif
