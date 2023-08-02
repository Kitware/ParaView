// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqSingleLogViewerWidget_h
#define pqSingleLogViewerWidget_h

#include "pqComponentsModule.h"
#include "pqLogViewerWidget.h"

#include "vtkSMProxy.h"
#include "vtkSmartPointer.h"

/**
 * @class pqSingleLogViewerWidget
 * @brief A single log viewer widget which has a reference to the log recorder proxy.
 */
class PQCOMPONENTS_EXPORT pqSingleLogViewerWidget : public pqLogViewerWidget
{
  Q_OBJECT
  using Superclass = pqLogViewerWidget;

public:
  /**
   * @param parent The parent of this widget.
   * @param logRecorderProxy has the information of server location,
   * and the rank setting indicates which rank whose log is shown in
   * this widget.
   * @param rank
   */
  pqSingleLogViewerWidget(QWidget* parent, vtkSmartPointer<vtkSMProxy> logRecorderProxy, int rank);
  ~pqSingleLogViewerWidget() override = default;

  /**
   * Refresh the log viewer
   */
  void refresh();

  /**
   * Get the log recorder proxy reference hold by this widget.
   */
  const vtkSmartPointer<vtkSMProxy>& getLogRecorderProxy() const;

  /**
   * Get the rank of the process whose log is shown in this widget.
   */
  int getRank() const;

  /**
   * Disable log recording of the specific rank when the widget is closed.
   */
  void closeEvent(QCloseEvent*) override;

private:
  Q_DISABLE_COPY(pqSingleLogViewerWidget);

  vtkSmartPointer<vtkSMProxy> LogRecorderProxy;
  int Rank;
};

#endif // pqSingleLogViewerWidget_h
