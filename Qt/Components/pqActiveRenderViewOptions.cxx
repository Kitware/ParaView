/*=========================================================================

   Program: ParaView
   Module:    pqActiveRenderViewOptions.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

/// \file pqActiveRenderViewOptions.cxx
/// \date 7/31/2007

#include "pqActiveRenderViewOptions.h"

#include "pqApplicationCore.h"
#include "pqRenderView.h"
#include "pqSettingsDialog.h"
#include "pqUndoStack.h"
#include "pqView.h"

#include <QWidget>


pqActiveRenderViewOptions::pqActiveRenderViewOptions(QObject *parentObject)
  : pqActiveViewOptions(parentObject)
{
}

pqActiveRenderViewOptions::~pqActiveRenderViewOptions()
{
}

void pqActiveRenderViewOptions::showOptions(pqView *view,
    QWidget *widgetParent)
{
  pqSettingsDialog dialog(widgetParent);
  dialog.setRenderView(qobject_cast<pqRenderView*>(view));
  pqUndoStack *stack = pqApplicationCore::instance()->getUndoStack();
  if(stack)
    {
    this->connect(&dialog, SIGNAL(beginUndo(const QString &)),
        stack, SLOT(beginUndoSet(const QString &)));
    this->connect(&dialog, SIGNAL(endUndo()), stack, SLOT(endUndoSet()));
    }

  dialog.exec();
  emit this->optionsClosed(this);
}

void pqActiveRenderViewOptions::changeView(pqView *)
{
}

void pqActiveRenderViewOptions::closeOptions()
{
}


