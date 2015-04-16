/*=========================================================================

   Program: ParaView
   Module:  pqRenderViewOneSelectionReaction.h

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
#ifndef __pqRenderViewOneSelectionReaction_h
#define __pqRenderViewOneSelectionReaction_h

#include "pqRenderViewSelectionReaction.h"
#include <QPointer>
#include <QCursor>

class pqRenderView;
class pqView;
class vtkIntArray;
class vtkObject;

/// pqRenderViewOneSelectionReaction handles various selection modes available on
/// RenderViews. This class handles only one selection at a time.
/// For interactive selection see pqRenderViewInteractiveSelectionReaction.
/// Simply create multiple instances of
/// pqRenderViewOneSelectionReaction to handle selection modes for that RenderView.
/// pqRenderViewOneSelectionReaction uses internal static members to ensure that
/// at most 1 view (and 1 type of selection) is in selection-mode at any given
/// time.
class PQAPPLICATIONCOMPONENTS_EXPORT pqRenderViewOneSelectionReaction :
  public pqRenderViewSelectionReaction
{
  Q_OBJECT
  typedef pqRenderViewSelectionReaction Superclass;
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
  pqRenderViewOneSelectionReaction(
    QAction* parentAction, pqRenderView* view, SelectionMode mode);
  virtual ~pqRenderViewOneSelectionReaction();

signals:
  void selectedCustomBox(int xmin, int ymin, int xmax, int ymax);
  void selectedCustomBox(const int region[4]);
  void selectedCustomPolygon(vtkIntArray* polygon);

public slots:
  /// starts the selection i.e. setup render view in selection mode.
  virtual bool beginSelection();

  /// finishes the selection. Doesn't cause the selection, just returns the
  /// render view to previous interaction mode.
  virtual bool endSelection();

private:
  /// callback called when the vtkPVRenderView is done with selection.
  void selectionChanged(vtkObject*, unsigned long, void* calldata);

private:
  Q_DISABLE_COPY(pqRenderViewOneSelectionReaction);
  SelectionMode Mode;
  unsigned long ObserverId;
  QCursor ZoomCursor;
};

#endif
