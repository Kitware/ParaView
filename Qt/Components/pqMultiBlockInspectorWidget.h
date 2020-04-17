/*=========================================================================

   Program: ParaView
   Module:  pqMultiBlockInspectorWidget.h

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
#ifndef pqMultiBlockInspectorWidget_h
#define pqMultiBlockInspectorWidget_h

#include "pqComponentsModule.h" // for exports
#include <QScopedPointer>       // for ivar
#include <QVariant>             // for ivar
#include <QWidget>

/**
 * @class pqMultiBlockInspectorWidget
 * @brief widget to show composite data hierarchy and controls its display properties.
 *
 * pqMultiBlockInspectorWidget is a QWidget that is used to allow user to view
 * a composite dataset hierarchy. It also supports viewing and modifying display
 * properties for the composite dataset.
 *
 * pqMultiBlockInspectorWidget monitors active pqActiveObjects to track active
 * port and view by default. To not have the pqMultiBlockInspectorWidget track the
 * active objects, you can pass `autotracking` as `false` to the constructor.
 * In that case, the port and view to use can be set using
 * `setOutputPort` and `setView`.
 *
 * @sa pqCompositeDataInformationTreeModel
 */

class pqDataRepresentation;
class pqOutputPort;
class pqView;
class QModelIndex;

class PQCOMPONENTS_EXPORT pqMultiBlockInspectorWidget : public QWidget
{
  Q_OBJECT
  typedef QWidget Superclass;

  Q_PROPERTY(QList<QVariant> blockVisibilities READ blockVisibilities WRITE setBlockVisibilities
      NOTIFY blockVisibilitiesChanged);

  Q_PROPERTY(
    QList<QVariant> blockColors READ blockColors WRITE setBlockColors NOTIFY blockColorsChanged);

  Q_PROPERTY(QList<QVariant> blockOpacities READ blockOpacities WRITE setBlockOpacities NOTIFY
      blockOpacitiesChanged);

  Q_PROPERTY(QList<QVariant> visibleBlocks READ visibleBlocks WRITE setVisibleBlocks NOTIFY
      blockOpacitiesChanged);

public:
  pqMultiBlockInspectorWidget(
    QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags(), bool autotracking = true);
  ~pqMultiBlockInspectorWidget() override;

  /**
   * Returns true if tracking of the active objects via pqActiveObjects is
   * enabled.
   *
   * It can be disabled by passing appropriate argument to the constructor.
   */
  bool isAutoTrackingEnabled() const { return this->AutoTracking; }

  //@{
  /**
   * Get/Set block visibilities. The value is a list of QVariants in pairs
   * where 1st value is the block index and 2nd value is its visibility
   * state.
   */
  QList<QVariant> blockVisibilities() const;
  void setBlockVisibilities(const QList<QVariant>& bvs);
  //@}

  //@{
  /**
   * Get/Set the visible blocks. Unlike blockVisibilities, this is compact list
   * of visible blocks given the current hierarchy.
   */
  QList<QVariant> visibleBlocks() const;
  void setVisibleBlocks(const QList<QVariant>& vbs);
  //@}

  //@{
  /**
   * Get/Set block colors. The value is a list of QVariants in 4-tuples where 1st
   * value is the block color and the next 3 values are rgb color in each in
   * range [0, 1.0].
   */
  QList<QVariant> blockColors() const;
  void setBlockColors(const QList<QVariant>& bcs);
  //@}

  //@{
  /**
   * Get/Set block opacities. The value is a list of QVariants in pairs where 1st
   * value is the block color and the 2nd value is opacity in range [0, 1.0].
   */
  QList<QVariant> blockOpacities() const;
  void setBlockOpacities(const QList<QVariant>& bos);
  //@}

  /**
   * Get the current output port. The widget shows the composite tree for the
   * data produced by at this port.
   */
  pqOutputPort* outputPort() const;

  /**
   * Get the current view.
   */
  pqView* view() const;

  /**
   * Get the current representation.
   */
  pqDataRepresentation* representation() const;

public Q_SLOTS:
  /**
   * When auto-tracking is disabled, sets the port to use to get the data
   * information for this widget to show.
   *
   * Calling this method when auto-tracking is disabled will raise a debug message
   * and has no effect.
   */
  void setOutputPort(pqOutputPort* port);

  /**
   * When auto-tracking is disabled, sets the view to use. If outputPort has a
   * representation is this view with block-coloring/opacity support, then controls
   * for coloring/opacity will be presented to the user.
   *
   * Calling this method when auto-tracking is disabled will raise a debug message
   * and has no effect.
   */
  void setView(pqView* view);

private Q_SLOTS:
  void setOutputPortInternal(pqOutputPort* port);
  void setViewInternal(pqView* view);
  void setRepresentation(pqDataRepresentation* repr);
  void selected(pqOutputPort* port);
  void modelDataChanged(const QModelIndex&, const QModelIndex&);
  void contextMenu(const QPoint&);
  void resetEventually();
  void resetNow();
  void itemDoubleClicked(const QModelIndex&);
  void updateRepresentation();
  void nameChanged();

  /**
   * Called when there's a potential for scalar coloring changes.
   */
  void updateScalarColoring();

Q_SIGNALS:
  void blockVisibilitiesChanged();
  void blockColorsChanged();
  void blockOpacitiesChanged();
  void requestRender();

private:
  //@{
  /**
   * Methods to change block colors or opacity ensemble.
   *
   * If the index is valid and identifies an item that is part of the current selection,
   * then all selected items get updated similarly. If not, only the chosen item is
   * updated. If the index is invalid, then all items part of the current
   * selection are updated.
   *
   * For setColor, if `newcolor` is invalid, then it acts as "reset color".
   * For setOpacity, if `opacity` is < 0, then it acts as "reset opacity".
   */
  void setColor(const QModelIndex& idx, const QColor& newcolor);
  void setOpacity(const QModelIndex& idx, double opacity);
  //@}

private:
  Q_DISABLE_COPY(pqMultiBlockInspectorWidget);

  class pqInternals;
  QScopedPointer<pqInternals> Internals;

  bool AutoTracking;
};

#endif
