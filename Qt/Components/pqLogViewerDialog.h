// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqLogViewerDialog_h
#define pqLogViewerDialog_h

#include "pqComponentsModule.h"
#include "pqSingleLogViewerWidget.h"

#include <QComboBox>
#include <QDialog>
#include <QMap>
#include <QPair>

#include "vtkLogger.h"
#include "vtkSMSession.h"

#include <array>

namespace Ui
{
class pqLogViewerDialog;
}

/**
 * @class pqLogViewerDialog
 *
 * @brief A window for showing multiple log viewers.
 *
 * This class displays logs generated with vtkPVLogger and reocorded
 * with vtkLogRecorder. Individual logs from client and server processes
 * are displayed in their own tabs in a QTabWidget in this window.
 */
class PQCOMPONENTS_EXPORT pqLogViewerDialog : public QDialog
{
  Q_OBJECT

public:
  pqLogViewerDialog(QWidget* parent = nullptr);
  ~pqLogViewerDialog() override;
  typedef QDialog Superclass;

  /**
   * Refresh the log viewers.
   */
  void refresh();

  /**
   * Clear the log viewers.
   */
  void clear();

  /**
   * Add a new log viewer according to settings in the UI.
   */
  void addLogView();

protected:
  // Override to handle custom close button icon in tab widget
  bool eventFilter(QObject* obj, QEvent* event) override;

private Q_SLOTS:
  void linkedScroll(double time);

  // Set the verbosity of logs on a given process
  void setProcessVerbosity(int process, int index);

private: // NOLINT(readability-redundant-access-specifiers)
  Q_DISABLE_COPY(pqLogViewerDialog)

  // Add a log view to the window
  void appendLogView(pqSingleLogViewerWidget* logView);

  void recordRefTimes();
  void initializeRankComboBox();
  void initializeVerbosityComboBoxes();
  void initializeVerbosities(QComboBox* combobox);

  void updateCategory(int category, bool promote);

  void updateCategories();

  // Convert combobox index to verbosity
  vtkLogger::Verbosity getVerbosity(int index);

  // Convert verbosity to combobox index
  int getVerbosityIndex(vtkLogger::Verbosity verbosity);

  Ui::pqLogViewerDialog* Ui;
  QList<pqSingleLogViewerWidget*> LogViews;
  QVector<int> RankNumbers;
  QList<vtkSmartPointer<vtkSMProxy>> LogRecorderProxies;
  using LogLocation = QPair<vtkSmartPointer<vtkSMProxy>, int>;
  QMap<LogLocation, double> RefTimes;
  std::array<bool, 6> CategoryPromoted;
};

#endif // pqLogViewerDialog_h
