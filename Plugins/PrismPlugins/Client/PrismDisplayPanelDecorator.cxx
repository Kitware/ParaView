/*=========================================================================

   Program: ParaView
   Module:    PrismDisplayPanelDecorator.cxx

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
#include "PrismDisplayPanelDecorator.h"

// ParaView Includes.
#include "pqDisplayPanel.h"
#include "pqActiveObjects.h"

//Qt Includes
#include <QGroupBox>

// Prism Includes
#include "PrismView.h"
//-----------------------------------------------------------------------------
PrismDisplayPanelDecorator::PrismDisplayPanelDecorator(pqDisplayPanel* panel)
  : Superclass(panel)
{
  PrismView* ren = qobject_cast<PrismView*>(
    pqActiveObjects::instance().activeView());
  if (ren)
    {
    //if we are in a prism view, we don't want to show the transform on the actors
    //as it is being set for world scaling. This is a hackish way of getting the
    //transform group box
    QGroupBox *transformGroup = panel->findChild<QGroupBox*>("TransformationGroup");
    transformGroup->hide();
    }
}

//-----------------------------------------------------------------------------
PrismDisplayPanelDecorator::~PrismDisplayPanelDecorator()
{
}
