/*=========================================================================

   Program: ParaView
   Module:  pqLogViewerWidget.h

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
#ifndef pqLogViewerWidget_h
#define pqLogViewerWidget_h

#include "pqCoreModule.h"

#include <QModelIndex>    // for QModelIndex
#include <QScopedPointer> // for QScopedPointer
#include <QWidget>        // for QWidget

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
  virtual ~pqLogViewerWidget() override;

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
  static QVector<QString> extractLogParts(const QStringRef& txt, bool& is_raw);

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
