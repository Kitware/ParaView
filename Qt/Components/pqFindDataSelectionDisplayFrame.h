// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqFindDataSelectionDisplayFrame_h
#define pqFindDataSelectionDisplayFrame_h

#include "pqComponentsModule.h"
#include <QWidget>

class pqOutputPort;
class pqView;

/**
 * pqFindDataSelectionDisplayFrame is designed to be used by pqFindDataDialog.
 * pqFindDataDialog uses this class to allow controlling the display properties
 * for the selection in the active view. Currently, it only support
 * controlling the display properties for the selection in a render view.
 * It monitors the active selection by tracking pqSelectionManager as well as
 * the active view by tracking pqActiveObjects singleton.
 */
class PQCOMPONENTS_EXPORT pqFindDataSelectionDisplayFrame : public QWidget
{
  Q_OBJECT
  typedef QWidget Superclass;

public:
  pqFindDataSelectionDisplayFrame(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags{});
  ~pqFindDataSelectionDisplayFrame() override;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Set the output port that is currently selected for which we are
   * controlling the selection display properties.
   */
  void setSelectedPort(pqOutputPort*);

  /**
   * set the view in which we are controlling the selection display properties.
   * label properties as well as which array to label with affect only the
   * active view.
   */
  void setView(pqView*);

private Q_SLOTS:
  void updatePanel();
  void editLabelPropertiesSelection();
  void editLabelPropertiesInteractiveSelection();
  void onDataUpdated();

private: // NOLINT(readability-redundant-access-specifiers)
  Q_DISABLE_COPY(pqFindDataSelectionDisplayFrame)

  void updateInteractiveSelectionLabelProperties();

  class pqInternals;
  friend class pqInternals;

  pqInternals* Internals;
};

#endif
