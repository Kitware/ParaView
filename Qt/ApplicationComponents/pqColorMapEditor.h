/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

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
#ifndef pqColorMapEditor_h
#define pqColorMapEditor_h

#include "pqApplicationComponentsModule.h"
#include <QWidget>

class vtkSMProxy;
class pqDataRepresentation;

/**
* pqColorMapEditor is a widget that can be used to edit the active color-map,
* if any. The panel is implemented as an auto-generated panel (similar to the
* Properties panel) that shows the properties on the lookup-table proxy.
* Custom widgets such as pqColorOpacityEditorWidget,
* pqColorAnnotationsPropertyWidget, and others are used to
* control certain properties on the proxy.
*/
class PQAPPLICATIONCOMPONENTS_EXPORT pqColorMapEditor : public QWidget
{
  Q_OBJECT
  typedef QWidget Superclass;

public:
  pqColorMapEditor(QWidget* parent = 0);
  ~pqColorMapEditor() override;

protected Q_SLOTS:
  /**
  * slot called to update the currently showing proxies.
  */
  void updateActive();

  /**
  * slot called to update the visible widgets.
  */
  void updatePanel();

  /**
  * render's view when transfer function is modified.
  */
  void renderViews();

  /**
  * Save the current transfer function(s) as default.
  */
  void saveAsDefault();

  /**
  * Save the current transfer function(s) as default for arrays with
  * the same name as the selected array.
  */
  void saveAsArrayDefault();

  /**
  * Restore the defaults (undoes effects of saveAsDefault()).
  */
  void restoreDefaults();

  /**
  * called when AutoUpdate button is toggled.
  */
  void setAutoUpdate(bool);

  void updateIfNeeded();

protected:
  void setDataRepresentation(pqDataRepresentation* repr);
  void setColorTransferFunction(vtkSMProxy* ctf);

protected Q_SLOTS:
  /**
  * update the enabled state for show/edit scalar bar buttons.
  */
  void updateScalarBarButtons();

private:
  Q_DISABLE_COPY(pqColorMapEditor)
  class pqInternals;
  pqInternals* Internals;
  friend class pqInternals;
};

#endif
