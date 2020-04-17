/*=========================================================================

   Program: ParaView
   Module:    $RCS $

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/
#ifndef _pqSetBreakpointDialog_h
#define _pqSetBreakpointDialog_h

#include "pqComponentsModule.h"
#include <QDialog>

namespace Ui
{
class pqSetBreakpointDialog;
}

class pqServer;
class QTreeWidget;
class pqPipelineSource;

/**
* Sets a breakpoint for a remote simulation. It allows a user to
* specify a time in the future (using simulation time or time step)
* when a simulation linked with Catalyst should pause.
*
* @ingroup LiveInsitu
*/
class PQCOMPONENTS_EXPORT pqSetBreakpointDialog : public QDialog
{
  Q_OBJECT

public:
  pqSetBreakpointDialog(QWidget* Parent);
  ~pqSetBreakpointDialog() override;

Q_SIGNALS:
  void breakpointHit();

protected Q_SLOTS:
  void onAccepted();
  void onTimeUpdated();

private:
  Q_DISABLE_COPY(pqSetBreakpointDialog)
  Ui::pqSetBreakpointDialog* const Ui;
};

#endif // !_pqSetBreakpointDialog_h
