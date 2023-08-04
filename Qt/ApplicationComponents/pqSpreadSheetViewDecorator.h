// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqSpreadSheetViewDecorator_h
#define pqSpreadSheetViewDecorator_h

#include "pqApplicationComponentsModule.h"
#include <QObject>
#include <QScopedPointer> // for QScopedPointer

class pqSpreadSheetView;
class pqOutputPort;
class pqDataRepresentation;

/**
 * pqSpreadSheetViewDecorator adds decoration to a spread-sheet view. This
 * includes widgets that allows changing the currently shown source/field etc.
 * To use the decorator, simply instantiate a new decorator for every new
 * instance of pqSpreadSheetView.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqSpreadSheetViewDecorator : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;
  Q_PROPERTY(bool allowChangeOfSource READ allowChangeOfSource WRITE setAllowChangeOfSource);

  ///@{
  /**
   * There properties are connected to the corresponding ServerManager
   * properties on the SpreadsheetView proxy.
   */
  Q_PROPERTY(
    bool generateCellConnectivity READ generateCellConnectivity WRITE setGenerateCellConnectivity);
  Q_PROPERTY(bool showFieldData READ showFieldData WRITE setShowFieldData);
  Q_PROPERTY(
    bool showSelectedElementsOnly READ showSelectedElementsOnly WRITE setShowSelectedElementsOnly);
  Q_PROPERTY(int fieldAssociation READ fieldAssociation WRITE setFieldAssociation);
  ///@}

public:
  pqSpreadSheetViewDecorator(pqSpreadSheetView* view);
  ~pqSpreadSheetViewDecorator() override;

  ///@{
  /**
   * These are linked to the corresponding properties on the SpreadsheetView
   * proxy using a pqPropertyLinks instance.
   */
  bool generateCellConnectivity() const;
  void setGenerateCellConnectivity(bool);
  bool showFieldData() const;
  void setShowFieldData(bool);
  bool showSelectedElementsOnly() const;
  void setShowSelectedElementsOnly(bool);
  int fieldAssociation() const;
  void setFieldAssociation(int);
  ///@}

  void setPrecision(int);
  void setFixedRepresentation(bool);

  /**
   * Returns whether the user should allowed to interactive change the source.
   * being shown in the view. `true` by default.
   */
  bool allowChangeOfSource() const;

  /**
   * Set whether the user should be allowed to change the source interactively.
   */
  void setAllowChangeOfSource(bool val);

  /**
   * Reimplemented to handle copy to clipboard in CSV format.
   */
  bool eventFilter(QObject* object, QEvent* e) override;

Q_SIGNALS:
  void uiModified();

protected Q_SLOTS:
  /**
   * Triggered when the field association attribute changed.
   * Enabled/disable buttons that apply only to a given attribute.
   */
  void onCurrentAttributeChange(int);

  void currentIndexChanged(pqOutputPort*);
  void showing(pqDataRepresentation*);
  void displayPrecisionChanged(int);
  void toggleFixedRepresentation(bool);
  void copyToClipboard();

protected: // NOLINT(readability-redundant-access-specifiers)
  pqSpreadSheetView* Spreadsheet;

private:
  Q_DISABLE_COPY(pqSpreadSheetViewDecorator)

  class pqInternal;
  QScopedPointer<pqInternal> Internal;
};

#endif
