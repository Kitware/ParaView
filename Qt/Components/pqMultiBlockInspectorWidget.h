// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqMultiBlockInspectorWidget_h
#define pqMultiBlockInspectorWidget_h

#include "pqComponentsModule.h" // for exports
#include <QScopedPointer>       // for ivar
#include <QVariant>             // for ivar
#include <QWidget>

/**
 * @class pqMultiBlockInspectorWidget
 * @brief widget to show composite data hierarchy and control its display properties.
 *
 * pqMultiBlockInspectorWidget is a QWidget that allows the user to explore the hierarchical
 * structure of from a composite dataset produced by a source.
 * Optionally, it also supports viewing and changing appearance parameters associated
 * with the hierarchy such as block visibility, block colors, and block opacity.
 *
 * pqMultiBlockInspectorWidget monitors active pqActiveObjects to track active
 * port and view by default. To not have the pqMultiBlockInspectorWidget track the
 * active objects, you can pass `autotracking` as `false` to the constructor.
 * In that case, the port and representation to use can be set using
 * `setOutputPort` and `setRepresentation`.
 *
 * @sa pqDataAssemblyTreeModel, pqDataAssemblyPropertyWidget
 */

class pqDataRepresentation;
class pqOutputPort;
class QModelIndex;

class PQCOMPONENTS_EXPORT pqMultiBlockInspectorWidget : public QWidget
{
  Q_OBJECT
  typedef QWidget Superclass;

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

  /**
   * Get the current output port. The widget shows the composite tree for the
   * data produced by at this port.
   */
  pqOutputPort* outputPort() const;

  /**
   * Get the current representation.
   */
  pqDataRepresentation* representation() const;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * When auto-tracking is disabled, sets the port to use to get the data
   * information for this widget to show.
   *
   * Calling this method when auto-tracking is disabled will raise a debug message
   * and has no effect.
   */
  void setOutputPort(pqOutputPort* port);

  /**
   * When auto-tracking is disabled, sets the representation to use.
   *
   * Calling this method when auto-tracking is disabled will raise a debug message
   * and has no effect.
   */
  void setRepresentation(pqDataRepresentation* view);

private:
  Q_DISABLE_COPY(pqMultiBlockInspectorWidget);

  class pqInternals;
  QScopedPointer<pqInternals> Internals;

  bool AutoTracking;
};

#endif
