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
#ifndef __pqPointSpriteControls_h
#define __pqPointSpriteControls_h

#include "pqPropertyWidget.h"
#include <QPointer>

class pqPipelineRepresentation;
class pqWidgetRangeDomain;
class vtkSMProperty;
class vtkSMPropertyGroup;

/// FIXME: pqPointSpriteControls currently simply duplicates code from
/// pqPointSpriteDisplayPanelDecorator. At some point we need to clean
/// everything up. The PointSprite representation does several non-standard
/// things, we need to fix those up too. For now, we're leaving them as is.
class pqPointSpriteControls : public pqPropertyWidget
{
  Q_OBJECT
  typedef pqPropertyWidget Superclass;
public:
  pqPointSpriteControls(
    vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup, QWidget* parentObject=0);
  virtual ~pqPointSpriteControls();

private:
  void initialize(pqPipelineRepresentation* repr);

private slots:
  void representationTypeChanged();

  void updateEnableState();

  // slots called when the radius array settings change
  void updateRadiusArray();

  // slots called when the alpha array settings change
  void updateOpacityArray();

  void  showRadiusDialog();
  void  showOpacityDialog();

  void  reloadGUI();

private:
  // setup the connections between the GUI and the proxies
  void setupGUIConnections();

  // called when the representation has been modified to update the menus
  void setRepresentation(pqPipelineRepresentation* repr);

  void  LinkWithRange(QWidget* widget, const char* signal, vtkSMProperty* prop,
    QPointer<pqWidgetRangeDomain>& widgetRangeDomain);

  class pqInternals;
  pqInternals* Internals;


private:
  Q_DISABLE_COPY(pqPointSpriteControls)
};

#endif
