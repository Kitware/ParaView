/*=========================================================================

   Program: ParaView
   Module:  pqRenameProxyReaction.cxx

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
#include "pqRenameProxyReaction.h"

#include "pqActiveObjects.h"
#include "pqProxy.h"
#include "pqView.h"

#include "vtkSMTrace.h"

#include <QInputDialog>
#include <QString>

//-----------------------------------------------------------------------------
pqRenameProxyReaction::pqRenameProxyReaction(QAction* renameAction, pqProxy* proxy)
  : Superclass(renameAction)
  , Proxy(proxy)
{
}

//-----------------------------------------------------------------------------
void pqRenameProxyReaction::onTriggered()
{
  bool ok;
  QString group = dynamic_cast<pqView*>(this->Proxy) ? "View" : "Proxy";
  QString oldName = this->Proxy->getSMName();
  QString newName = QInputDialog::getText(pqActiveObjects::instance().activeView()->widget(),
    tr("Rename") + " " + group + "...", tr("New name:"), QLineEdit::Normal, oldName, &ok);

  if (ok && !newName.isEmpty() && newName != oldName)
  {
    if (group == "View")
    {
      SM_SCOPED_TRACE(CallFunction)
        .arg("RenameView")
        .arg(newName.toLocal8Bit().data())
        .arg((vtkObject*)this->Proxy->getProxy());
    }
    else
    {
      SM_SCOPED_TRACE(CallFunction)
        .arg("RenameProxy")
        .arg(newName.toLocal8Bit().data())
        .arg(this->Proxy->getSMGroup().toLocal8Bit().data())
        .arg((vtkObject*)this->Proxy->getProxy());
    }

    this->Proxy->rename(newName);
  }
}
