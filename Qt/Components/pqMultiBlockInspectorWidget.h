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
