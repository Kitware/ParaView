// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqAdjustToCurrentComboBox_h
#define pqAdjustToCurrentComboBox_h

#include "pqWidgetsModule.h"
#include <QComboBox>

/**
 * pqAdjustToCurrentComboBox is a QComboBox specialization whose size hint is
 * based on the width needed to display the currently selected item, rather
 * than the widest item in the model.
 *
 * QComboBox::AdjustToContents (and AdjustToContentsOnFirstShow) size the
 * combobox to fit the widest entry in the list, so the widget never shrinks
 * back down once a long item has been added, even if a short item is
 * currently selected. pqAdjustToCurrentComboBox instead recomputes its size
 * hint from the current item alone, so the combobox grows and shrinks as the
 * selection changes.
 */
class PQWIDGETS_EXPORT pqAdjustToCurrentComboBox : public QComboBox
{
  Q_OBJECT
  typedef QComboBox Superclass;

public:
  pqAdjustToCurrentComboBox(QWidget* parent = nullptr);
  ~pqAdjustToCurrentComboBox() override = default;

  QSize sizeHint() const override;
  QSize minimumSizeHint() const override;

private:
  Q_DISABLE_COPY(pqAdjustToCurrentComboBox);
};

#endif
