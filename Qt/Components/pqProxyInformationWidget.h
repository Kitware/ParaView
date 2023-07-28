// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqProxyInformationWidget_h
#define pqProxyInformationWidget_h

#include "pqComponentsModule.h"
#include <QScopedPointer>
#include <QWidget>

class pqOutputPort;
class vtkPVDataInformation;

/**
 * @class pqProxyInformationWidget
 * @brief Widget to show information about data produced by an algorithm
 *
 * pqProxyInformationWidget is intended to show meta-data about the data
 * produced by a VTK algorithm. In ParaView, that maps to representing
 * meta-data available in `vtkPVDataInformation`.
 *
 * pqProxyInformationWidget uses `pqActiveObjects` to monitor the active
 * output-port and updates to show data-information for the data produced by the
 * active output-port, if any.
 *
 */
class PQCOMPONENTS_EXPORT pqProxyInformationWidget : public QWidget
{
  Q_OBJECT;
  using Superclass = QWidget;

public:
  pqProxyInformationWidget(QWidget* p = nullptr);
  ~pqProxyInformationWidget() override;

  /**
   * Returns the output port from whose data information this widget is
   * currently showing.
   */
  pqOutputPort* outputPort() const;

  /**
   * Returns the `vtkPVDataInformation` currently shown.
   */
  vtkPVDataInformation* dataInformation() const;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Set the display whose properties we want to edit.
   */
  void setOutputPort(pqOutputPort* outputport);

private Q_SLOTS:

  /**
   * Updates the UI using current vtkPVDataInformation.
   */
  void updateUI();
  void updateSubsetUI();

private: // NOLINT(readability-redundant-access-specifiers)
  Q_DISABLE_COPY(pqProxyInformationWidget);

  class pqInternals;
  QScopedPointer<pqInternals> Internals;
};

#endif
