// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqSelectionLinkDialog_h
#define pqSelectionLinkDialog_h

#include "pqComponentsModule.h"
#include <QDialog>

/**
 * Dialog used to ask the user for selection link options.
 */
class PQCOMPONENTS_EXPORT pqSelectionLinkDialog : public QDialog
{
  Q_OBJECT
  typedef QDialog Superclass;

public:
  pqSelectionLinkDialog(QWidget* parent, Qt::WindowFlags f = Qt::WindowFlags{});
  ~pqSelectionLinkDialog() override;

  /**
   * Specify if the convert to indices flag should be set to on
   * Default behaviour is on. It can sometimes be useful to disable it,
   * eg. A frustum selection over multiple datasets.
   */
  void setEnableConvertToIndices(bool enable);

  /**
   * Returns if the user requested to convert to indices.
   */
  bool convertToIndices() const;

private:
  Q_DISABLE_COPY(pqSelectionLinkDialog)

  class pqInternal;
  pqInternal* Internal;
};

#endif
