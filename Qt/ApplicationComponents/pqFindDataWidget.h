// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqFindDataWidget_h
#define pqFindDataWidget_h

#include "pqApplicationComponentsModule.h" // for exports
#include <QScopedPointer>                  // for QScopedPointer
#include <QWidget>

/**
 * @class pqFindDataWidget
 * @brief Widget to find data using selection queries.
 *
 * @section DeveloperNotes Developer Notes
 *
 * * Currently, this simply uses `pqFindDataCurrentSelectionFrame` to show
 *   current selection. We should modernize/cleanup that code and maybe just
 *   merge that code with this class to avoid confusion.
 *
 * * Currently, this simply uses `pqFindDataSelectionDisplayFrame` to allow
 *   editing current selection's display properties. We should modernize that
 *   code and maybe just move it to this class to avoid confusion.
 */

class pqServer;
class pqPipelineSource;
class pqOutputPort;

class PQAPPLICATIONCOMPONENTS_EXPORT pqFindDataWidget : public QWidget
{
  Q_OBJECT
  typedef QWidget Superclass;

public:
  pqFindDataWidget(QWidget* parent = nullptr);
  ~pqFindDataWidget() override;

  pqServer* server() const;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Set the server to use.
   */
  void setServer(pqServer* server);

private Q_SLOTS:
  /**
   * Ensure we don't have any references to ab proxy being deleted.
   */
  void aboutToRemove(pqPipelineSource*);

private: // NOLINT(readability-redundant-access-specifiers)
  Q_DISABLE_COPY(pqFindDataWidget);

  class pqInternals;
  QScopedPointer<pqInternals> Internals;

  void handleSelectionChanged(pqOutputPort*);
};

#endif
