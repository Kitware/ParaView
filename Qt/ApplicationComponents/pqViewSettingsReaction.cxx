/*=========================================================================

   Program: ParaView
   Module:    pqViewSettingsReaction.cxx

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
#include "pqViewSettingsReaction.h"

#include "pqActiveObjects.h"
#include "pqViewSettingsManager.h"
#include "pqView.h"

QPointer<pqViewSettingsManager> pqViewSettingsReaction::Manager;
int pqViewSettingsReaction::Count = 0;

//-----------------------------------------------------------------------------
pqViewSettingsReaction::pqViewSettingsReaction(QAction* parentObject,
  pqView* view /*=0*/)
  : Superclass(parentObject), View(view)
{
  pqViewSettingsReaction::Count++;
  if (!view)
    {
    QObject::connect(&pqActiveObjects::instance(), SIGNAL(viewChanged(pqView*)),
      this, SLOT(updateEnableState()), Qt::QueuedConnection);
    }
  this->updateEnableState();
}

//-----------------------------------------------------------------------------
pqViewSettingsReaction::~pqViewSettingsReaction()
{
  pqViewSettingsReaction::Count--;
  if (pqViewSettingsReaction::Count == 0)
    {
    delete pqViewSettingsReaction::Manager;
    }
}

//-----------------------------------------------------------------------------
pqViewSettingsManager* pqViewSettingsReaction::GetManager()
{
  if (!pqViewSettingsReaction::Manager)
    {
    pqViewSettingsReaction::Manager = new pqViewSettingsManager();
    }
  return pqViewSettingsReaction::Manager;
}

//-----------------------------------------------------------------------------
void pqViewSettingsReaction::updateEnableState()
{
  pqView *view = this->View;
  if (!view)
    {
    view = pqActiveObjects::instance().activeView();
    }
  if (view && this->GetManager()->canShowOptions(view))
    {
    this->parentAction()->setEnabled(true);
    }
  else
    {
    this->parentAction()->setEnabled(false);
    }
}

//-----------------------------------------------------------------------------
void pqViewSettingsReaction::showViewSettingsDialog()
{
  pqViewSettingsReaction::GetManager()->showOptions();
}

