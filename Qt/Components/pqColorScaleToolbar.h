/*=========================================================================

   Program: ParaView
   Module:    pqColorScaleToolbar.h

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

#ifndef _pqColorScaleToolbar_h
#define _pqColorScaleToolbar_h

#include "pqComponentsExport.h"
#include <QObject>

class pqColorScaleToolbarInternal;
class pqDataRepresentation;
class pqDisplayColorWidget;
class QAction;

/// TO_DEPRECATE: Remove this class since it's not longer of any use. The
/// functionality has been split into reactions for handling the actions from the
/// color toolbar.
class PQCOMPONENTS_EXPORT pqColorScaleToolbar : public QObject
{
  Q_OBJECT

public:
  pqColorScaleToolbar(QObject *parent=0);
  virtual ~pqColorScaleToolbar();

  /// Sets the color map editor/color chooser tool button.
  void setColorAction(QAction *action);

  /// Sets the rescale to data range tool button.
  void setRescaleAction(QAction *action);

  /// Sets the color by widget.
  void setColorWidget(pqDisplayColorWidget *widget);

public slots:
  /// Sets the active representation for the color scale buttons.
  void setActiveRepresentation(pqDataRepresentation *display);

  /// Shows the edit color map dialog.
  void editColorMap(pqDataRepresentation *display);

  /// Changes the color or color map.
  void changeColor();

  /// Rescales to the data range.
  void rescaleRange();

private:
  pqColorScaleToolbarInternal *Internal;
  QAction *ColorAction;
  QAction *RescaleAction;
};

#endif
