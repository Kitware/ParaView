/*=========================================================================

   Program: ParaView
   Module:    pqSelectionReaction.cxx

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
#include "pqSelectionReaction.h"
#include "pqView.h"
#include <QActionGroup>

//-----------------------------------------------------------------------------
int pqSelectionReaction::getSelectionModifier()
{
  if (this->ModifierGroup)
  {
    // we cannot use QActionGroup::checkedAction() since the ModifierGroup may
    // not be exclusive.
    foreach (QAction* maction, this->ModifierGroup->actions())
    {
      if (maction->isChecked() && maction->data().isValid())
      {
        return maction->data().toInt();
      }
    }
  }
  return pqView::PV_SELECTION_DEFAULT;
}

void pqSelectionReaction::uncheckSelectionModifiers()
{
  if (this->ModifierGroup)
  {
    foreach (QAction* act, this->ModifierGroup->actions())
    {
      act->setChecked(false);
    }
  }
}

void pqSelectionReaction::disableSelectionModifiers(bool disable)
{
  if (this->ModifierGroup)
  {
    this->ModifierGroup->setEnabled(!disable);
    if (disable)
    {
      this->uncheckSelectionModifiers();
    }
  }
}
//-----------------------------------------------------------------------------
pqSelectionReaction::pqSelectionReaction(QAction* parentObject, QActionGroup* modifierGroup)
  : Superclass(parentObject)
  , ModifierGroup(modifierGroup)
{
  // if modified is changed while selection is in progress, we need to ensure we
  // update the selection modifier on the view.
  if (modifierGroup)
  {
    this->connect(modifierGroup, SIGNAL(triggered(QAction*)), SLOT(modifiersChanged()));
  }
}
