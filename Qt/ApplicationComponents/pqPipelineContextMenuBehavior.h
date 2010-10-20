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
#ifndef __pqPipelineContextMenuBehavior_h
#define __pqPipelineContextMenuBehavior_h

#include <QObject>
#include <QPoint> // needed for QPoint.
#include <QPointer>
#include "pqApplicationComponentsExport.h"

class pqDataRepresentation;
class pqPipelineRepresentation;
class pqView;
class QAction;
class QMenu;

/// @ingroup Behaviors
///
/// This behavior manages showing up of a context menu with sensible pipeline
/// related actions for changing color/visibility etc. when the user
/// right-clicks on top of an object in the 3D view. Currently, it only supports
/// views with proxies that vtkSMRenderViewProxy subclasses.
class PQAPPLICATIONCOMPONENTS_EXPORT pqPipelineContextMenuBehavior : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;
public:
  pqPipelineContextMenuBehavior(QObject* parent=0);
  virtual ~pqPipelineContextMenuBehavior();

protected slots:
  /// Called when a new view is added. We add actions to the widget for context
  /// menu if the view is a render-view.
  void onViewAdded(pqView*);

  void hide();
  void reprTypeChanged(QAction* action);
  void colorMenuTriggered(QAction* action);

protected:
  virtual void buildMenu(pqDataRepresentation* repr);
  virtual void buildColorFieldsMenu(
    pqPipelineRepresentation* pipelineRepr, QMenu* menu);

  virtual bool eventFilter(QObject* caller, QEvent* e);

  QMenu* Menu;
  QPoint Position;
  QPointer<pqDataRepresentation> PickedRepresentation;
private:
  Q_DISABLE_COPY(pqPipelineContextMenuBehavior)

};

#endif
