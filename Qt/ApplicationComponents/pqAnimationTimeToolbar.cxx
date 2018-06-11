/*=========================================================================

   Program: ParaView
   Module:    pqAnimationTimeToolbar.cxx

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
#include "pqAnimationTimeToolbar.h"

#include "pqAnimationManager.h"
#include "pqAnimationScene.h"
#include "pqAnimationTimeWidget.h"
#include "pqCoreUtilities.h"
#include "pqPVApplicationCore.h"
#include "vtkCommand.h"
#include "vtkPVGeneralSettings.h"

//-----------------------------------------------------------------------------
void pqAnimationTimeToolbar::constructor()
{
  this->setWindowTitle("Current Time Controls");
  this->AnimationTimeWidget = new pqAnimationTimeWidget(this);
  this->AnimationTimeWidget->setPlayModeReadOnly(true);
  this->addWidget(this->AnimationTimeWidget);
  this->connect(pqPVApplicationCore::instance()->animationManager(),
    SIGNAL(activeSceneChanged(pqAnimationScene*)), SLOT(setAnimationScene(pqAnimationScene*)));
  pqCoreUtilities::connect(vtkPVGeneralSettings::GetInstance(), vtkCommand::ModifiedEvent, this,
    SLOT(updateTimeDisplay()));
}

//-----------------------------------------------------------------------------
void pqAnimationTimeToolbar::setAnimationScene(pqAnimationScene* scene)
{
  this->AnimationTimeWidget->setAnimationScene(scene ? scene->getProxy() : NULL);
}

//-----------------------------------------------------------------------------
void pqAnimationTimeToolbar::updateTimeDisplay()
{
  if (this->AnimationTimeWidget->timePrecision() !=
    vtkPVGeneralSettings::GetInstance()->GetAnimationTimePrecision())
  {
    this->AnimationTimeWidget->setTimePrecision(
      vtkPVGeneralSettings::GetInstance()->GetAnimationTimePrecision());
  }

  this->AnimationTimeWidget->setTimeNotation(
    vtkPVGeneralSettings::GetInstance()->GetAnimationTimeNotation());
}

//-----------------------------------------------------------------------------
pqAnimationTimeWidget* pqAnimationTimeToolbar::animationTimeWidget() const
{
  return this->AnimationTimeWidget;
}
