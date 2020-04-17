/*=========================================================================

   Program: ParaView
   Module:  pqLogViewerDialog.h

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

private:
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
  QList<vtkSmartPointer<vtkSMProxy> > LogRecorderProxies;
  using LogLocation = QPair<vtkSmartPointer<vtkSMProxy>, int>;
  QMap<LogLocation, double> RefTimes;
  std::array<bool, 5> CategoryPromoted;
};

#endif // pqLogViewerDialog_h
