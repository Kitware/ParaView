// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include <QDockWidget>

/**
 * pqWriterDockPanel is a simple dock widget that create a custom proxy and
 * a proxy widget for it.
 */
class pqWriterDockPanel : public QDockWidget
{
  Q_OBJECT
  typedef QDockWidget Superclass;

public:
  pqWriterDockPanel(
    const QString& title, QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());
  pqWriterDockPanel(QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());

private
  Q_SLOT : void constructor();
};
