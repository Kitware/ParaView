/*=========================================================================

   Program: ParaView
   Module:    pqColorToolbar.cxx

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
#include "pqColorToolbar.h"
#include "ui_pqColorToolbar.h"

#include "pqActiveObjects.h"
#include "pqDisplayColorWidget.h"
#include "pqEditColorMapReaction.h"
#include "pqResetScalarRangeReaction.h"
#include "pqScalarBarVisibilityReaction.h"
#include "pqSetName.h"

//-----------------------------------------------------------------------------
void pqColorToolbar::constructor()
{
  Ui::pqColorToolbar ui;
  ui.setupUi(this);

  new pqScalarBarVisibilityReaction(ui.actionScalarBarVisibility);
  new pqEditColorMapReaction(ui.actionEditColorMap);
  new pqResetScalarRangeReaction(ui.actionResetRange, true, pqResetScalarRangeReaction::DATA);
  new pqResetScalarRangeReaction(
    ui.actionRescaleCustomRange, true, pqResetScalarRangeReaction::CUSTOM);
  new pqResetScalarRangeReaction(
    ui.actionRescaleTemporalRange, true, pqResetScalarRangeReaction::TEMPORAL);
  new pqResetScalarRangeReaction(
    ui.actionRescaleVisibleRange, true, pqResetScalarRangeReaction::VISIBLE);

  pqDisplayColorWidget* display_color = new pqDisplayColorWidget(this) << pqSetName("displayColor");
  this->addWidget(display_color);

  QObject::connect(&pqActiveObjects::instance(),
    SIGNAL(representationChanged(pqDataRepresentation*)), display_color,
    SLOT(setRepresentation(pqDataRepresentation*)));
}
