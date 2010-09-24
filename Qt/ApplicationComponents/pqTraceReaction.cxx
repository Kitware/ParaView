/*=========================================================================

   Program: ParaView
   Module:    pqTraceReaction.cxx

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
#include "pqTraceReaction.h"

#include "pqPVApplicationCore.h"
#include "pqPythonManager.h"

//-----------------------------------------------------------------------------
pqTraceReaction::pqTraceReaction(QAction* parentObject, bool _start)
  : Superclass(parentObject)
{
  this->Start = _start;
  this->enable(_start);

  pqPythonManager *pythonManager = pqPVApplicationCore::instance()->pythonManager();

  if(pythonManager)
    {
    if (this->Start)
      {
      QObject::connect(pythonManager, SIGNAL(canStartTrace(bool)),
        this, SLOT(enable(bool)));
      }
    else
      {
      QObject::connect(pythonManager, SIGNAL(canStopTrace(bool)),
        this, SLOT(enable(bool)));
      }
    }
  else
    {
    // No python available
    this->enable(false);
    }
}

//-----------------------------------------------------------------------------
void pqTraceReaction::start()
{
  pqPythonManager *pythonManager = pqPVApplicationCore::instance()->pythonManager();
  if(!pythonManager)
    {
    qCritical("No application wide python manager.");
    return;
    }
  pythonManager->startTrace();
}

//-----------------------------------------------------------------------------
void pqTraceReaction::stop()
{
  pqPythonManager *pythonManager = pqPVApplicationCore::instance()->pythonManager();
  if(!pythonManager)
    {
    qCritical("No application wide python manager.");
    return;
    }
  pythonManager->stopTrace();
}

//-----------------------------------------------------------------------------
void pqTraceReaction::enable(bool canDoAction)
{
  this->parentAction()->setEnabled(canDoAction);
}

//-----------------------------------------------------------------------------
void pqTraceReaction::setLabel(const QString& label)
{
  if (this->Start)
    {
    this->parentAction()->setText(
      label.isEmpty() ? tr("Can't start trace") : tr("Start trace"));
    this->parentAction()->setStatusTip(
      label.isEmpty() ? tr("Can't start trace") : tr("Start trace"));
    }
  else
    {
    this->parentAction()->setText(
      label.isEmpty() ? tr("Can't stop trace") : tr("Stop trace"));
    this->parentAction()->setStatusTip(
      label.isEmpty() ? tr("Can't stop trace") : tr("Stop trace"));
    }
}
