/*=========================================================================

   Program: ParaView
   Module:    pqSelection3DHelper.h

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

#ifndef __pqSelection3DHelper_h
#define __pqSelection3DHelper_h

#include "pqApplicationComponentsModule.h"
#include "pqRubberBandHelper.h"
#include <QAction>

/*! \brief Utility to manage selections 3D views.
 *
 * pqSelection3DHelper is a class which handle different types of
 * selections for 3D-Views. It uses the pqRubberBandHelper as the
 * backend but sets up all the relevant signals and slots to enable
 * behavior whereby it ensures that only one of the selection modes is
 * enabled at any given time . Actions which represent GUI buttons for
 * the corresponding selection modes are set using the public API. At
 * any time only one of these actions/buttons is enabled based on
 * whatever is action/button is clicked (pressed) in the GUI. Actions
 * get disabled once the selection is performed in the 3D-View or when
 * the action/button is clicked again (released).
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqSelection3DHelper : public pqRubberBandHelper
{
  Q_OBJECT
  typedef pqRubberBandHelper Superclass;
public:
  pqSelection3DHelper(QObject* parent=NULL);
  virtual ~pqSelection3DHelper();

  /// Sets the SelectionMode Action
  void setActionSelectionMode(QAction *action);

  /// Sets the SelectSurfacePoints Action
  void setActionSelectSurfacePoints(QAction *action);

  /// Sets the Select_Frustum Action
  void setActionSelect_Frustum(QAction *action);

  /// Sets the SelectFrustumPoints Action
  void setActionSelectFrustumPoints(QAction *action);

  /// Sets the Select_Block Action
  void setActionSelect_Block(QAction *action);

  /// Sets the PickObject Action
  void setActionPickObject(QAction *action);

  /// Sets the SelectPolygonPoints Action
  void setActionSelectPolygonPoints(QAction *action);

private slots:
  /// toggles selection modes based on the current state.
  void toggleSurfaceSelection();
  void toggleSurfacePointsSelection();
  void toggleFrustumSelection();
  void toggleFrustumPointsSelection();
  void toggleBlockSelection();
  void togglePick();
  void togglePolygonPointsSelection();

  /// Based on which mode is selected this slot toggels the internal
  /// state between activation (pressed) and deactivation (release).
  void onSelectionModeChanged(int mode);

private:
  QActionGroup *ModeGroup;

  QAction *ActionPickObject;
  QAction *ActionSelect_Block;
  QAction *ActionSelectFrustumPoints;
  QAction *ActionSelect_Frustum;
  QAction *ActionSelectSurfacePoints;
  QAction *ActionSelectionMode;
  QAction *ActionSelectPolygonPoints;

  bool PickObject;
  bool Select_Block;
  bool SelectFrustumPoints;
  bool Select_Frustum;
  bool SelectSurfacePoints;
  bool SelectionMode;
  bool SelectPolygonPoints;
};

#endif
