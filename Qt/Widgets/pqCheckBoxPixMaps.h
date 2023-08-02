// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqCheckBoxPixMaps_h
#define pqCheckBoxPixMaps_h

#include "pqWidgetsModule.h"
#include <QObject>
#include <QPixmap>

class QWidget;

/**
 * pqCheckBoxPixMaps is a helper class that can used to create pixmaps for
 * checkboxs in various states. This is useful for showing checkboxes in qt-views.
 */
class PQWIDGETS_EXPORT pqCheckBoxPixMaps : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  /**
   * parent cannot be nullptr.
   */
  pqCheckBoxPixMaps(QWidget* parent);

  /**
   * Returns a pixmap for the given state .
   */
  QPixmap getPixmap(Qt::CheckState state, bool active) const;
  QPixmap getPixmap(int state, bool active) const
  {
    return this->getPixmap(static_cast<Qt::CheckState>(state), active);
  }

private:
  Q_DISABLE_COPY(pqCheckBoxPixMaps)

  enum PixmapStateIndex
  {
    Checked = 0,
    PartiallyChecked = 1,
    UnChecked = 2,

    // All active states in lower half
    Checked_Active = 3,
    PartiallyChecked_Active = 4,
    UnChecked_Active = 5,

    PixmapCount = 6
  };
  QPixmap Pixmaps[6];
};

#endif
