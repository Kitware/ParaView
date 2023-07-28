// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqLogViewerWidget_h
#define pqLogViewerWidget_h

#include "pqCoreModule.h"

#include <QModelIndex>    // for QModelIndex
#include <QScopedPointer> // for QScopedPointer
#include <QString>
#include <QVector>
#include <QWidget> // for QWidget

/**
 * @class pqLogViewerWidget
 *
 * @brief Provides a treeview with scoped logs along with a filtering
 * capability to restrict which logs are shown.
 */
class PQCORE_EXPORT pqLogViewerWidget : public QWidget
{
  Q_OBJECT
  using Superclass = QWidget;

public:
  pqLogViewerWidget(QWidget* parent = nullptr);
  ~pqLogViewerWidget() override;

  /**
   * Set the contents of the log viewer to the provided txt.
   */
  void setLog(const QString& text);

  /**
   * Append text to log.
   */
  void appendLog(const QString& text);

  /**
   * Manually set the filter wildcard. Mainly used for the global filter. Can be
   * overridden by the log-specific wildcard.
   */
  void setFilterWildcard(QString wildcard);

  /**
   * Scroll to the log near the specific time.
   */
  void scrollToTime(double time);

  /**
   * Utility function to parse a line of log into parts.
   * @param txt One line of the log
   * @param is_raw Return parameter stating whether the log is in the log format or just raw text.
   * @return A QVector containing different parts of the log.
   */
  static QVector<QString> extractLogParts(const QString& txt, bool& is_raw);

  /**
   * Update log table column visibilities.
   */
  void updateColumnVisibilities();

Q_SIGNALS:
  // Emitted when the widget is closed
  void closed();

  // \brief Emits when the scroll bar value changes, with the time of the current top log.
  // \param time
  void scrolled(double time);

protected Q_SLOTS:
  void toggleAdvanced();

  void exportLog();

private:
  Q_DISABLE_COPY(pqLogViewerWidget);

  class pqInternals;
  QScopedPointer<pqInternals> Internals;

  bool Advanced = false;
};

#endif
