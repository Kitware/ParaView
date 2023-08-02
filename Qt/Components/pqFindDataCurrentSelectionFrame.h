// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqFindDataCurrentSelectionFrame_h
#define pqFindDataCurrentSelectionFrame_h

#include "pqComponentsModule.h"
#include <QWidget>

class pqOutputPort;

/**
 * pqFindDataCurrentSelectionFrame is designed to be used by pqFindDataDialog.
 * pqFindDataDialog uses this class to show the current selection in a
 * spreadsheet view. This class encapsulates the logic to monitor the current
 * selection by tracking the pqSelectionManager and then showing the results in
 * the spreadsheet.
 */
class PQCOMPONENTS_EXPORT pqFindDataCurrentSelectionFrame : public QWidget
{
  Q_OBJECT
  typedef QWidget Superclass;

public:
  pqFindDataCurrentSelectionFrame(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags{});
  ~pqFindDataCurrentSelectionFrame() override;

  /**
   * return the port from which this frame is showing the selected data, if
   * any.
   */
  pqOutputPort* showingPort() const;

Q_SIGNALS:
  /**
   * signal fired to indicate the selected port that currently being shown in
   * the frame.
   */
  void showing(pqOutputPort*);

private Q_SLOTS:
  /**
   * show the selected data from the given output port in the frame.
   */
  void showSelectedData(pqOutputPort*);

  /**
   * update the field-type set of the internal spreadsheet view based on the
   * value in the combo-box.
   */
  void updateFieldType();

  /**
   * update the spreadsheet to show field data associated to points / cells
   */
  void showFieldData(bool show);

  /**
   * set the value for the "invert selection" property on the extract-selection
   * source to the one specified.
   */
  void invertSelection(bool);

  /**
   * update the data shown in the spreadsheet aka render the spreadsheet.
   */
  void updateSpreadSheet();

private: // NOLINT(readability-redundant-access-specifiers)
  Q_DISABLE_COPY(pqFindDataCurrentSelectionFrame)

  class pqInternals;
  friend class pqInternals;

  pqInternals* Internals;
};

#endif
