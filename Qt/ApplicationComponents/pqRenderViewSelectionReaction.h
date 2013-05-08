/*=========================================================================

   Program: ParaView
   Module:  pqRenderViewSelectionReaction.h

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
#ifndef __pqRenderViewSelectionReaction_h
#define __pqRenderViewSelectionReaction_h

#include "pqReaction.h"
#include <QPointer>
#include <QCursor>

class pqRenderView;
class pqView;
class vtkIntArray;
class vtkObject;

/// pqRenderViewSelectionReaction handles various selection modes available on
/// RenderViews. Simply create multiple instances of
/// pqRenderViewSelectionReaction to handle selection modes for that RenderView.
/// pqRenderViewSelectionReaction uses internal static members to ensure that
/// at most 1 view (and 1 type of selection) is in selection-mode at any given
/// time.
class PQAPPLICATIONCOMPONENTS_EXPORT pqRenderViewSelectionReaction :
  public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;
public:
  enum SelectionMode
    {
    SELECT_SURFACE_CELLS,
    SELECT_SURFACE_POINTS,
    SELECT_FRUSTUM_CELLS,
    SELECT_FRUSTUM_POINTS,
    SELECT_SURFACE_CELLS_POLYGON,
    SELECT_SURFACE_POINTS_POLYGON,
    SELECT_BLOCKS,
    SELECT_CUSTOM_BOX,
    SELECT_CUSTOM_POLYGON,
    ZOOM_TO_BOX
    };

  /// If \c view is NULL, this reaction will track the active-view maintained by
  /// pqActiveObjects.
  pqRenderViewSelectionReaction(
    QAction* parentAction, pqRenderView* view, SelectionMode mode);
  virtual ~pqRenderViewSelectionReaction();

signals:
  void selectedCustomBox(int xmin, int ymin, int xmax, int ymax);
  void selectedCustomBox(const int region[4]);
  void selectedCustomPolygon(vtkIntArray* polygon);

private slots:
  virtual void actionTriggered(bool val) 
    {
    if (val) { this->beginSelection(); }
    else { this->endSelection(); }
    }

  /// Called when this object was created with NULL as the view and the active
  /// view changes.
  void setView(pqView* view);

  /// starts the selection i.e. setup render view in selection mode.
  void beginSelection();

  /// finishes the selection. Doesn't cause the selection, just returns the
  /// render view to previous interaction mode.
  void endSelection();

private:
  /// callback called when the vtkPVRenderView is done with selection.
  void selectionChanged(vtkObject*, unsigned long, void* calldata);

private:
  Q_DISABLE_COPY(pqRenderViewSelectionReaction);
  QPointer<pqRenderView> View;
  SelectionMode Mode;
  int PreviousRenderViewMode;
  unsigned long ObserverId;
  QCursor ZoomCursor;

  static QPointer<pqRenderViewSelectionReaction> ActiveReaction;
};

#endif
