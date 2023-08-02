// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqDisplayColorWidget_h
#define pqDisplayColorWidget_h

#include "pqComponentsModule.h"

#include <QPair>
#include <QPointer>
#include <QWidget>

class pqDataRepresentation;
class pqScalarsToColors;
class QComboBox;
class vtkEventQtSlotConnect;
class vtkSMProxy;
class vtkSMViewProxy;

/**
 * pqDisplayColorWidget is a widget that can be used to select the array to
 * with for representations (also known as displays). It comprises of two
 * combo-boxes, one for the array name, and another for component number.
 * To use, set the representation using setRepresentation(..). If the
 * representation has appropriate properties, this widget will allow users
 * to select the array name.
 */
class PQCOMPONENTS_EXPORT pqDisplayColorWidget : public QWidget
{
  Q_OBJECT
  Q_PROPERTY(QString representationText READ representationText WRITE setRepresentationText);
  typedef QWidget Superclass;

public:
  typedef QPair<int, QString> ValueType;

  pqDisplayColorWidget(QWidget* parent = nullptr);
  ~pqDisplayColorWidget() override;

  /**
   * Get/Set the array name as pair (association-number, arrayname).
   */
  ValueType arraySelection() const;
  QString getCurrentText() const { return this->arraySelection().second; }

  /**
   * Get/Set the component number (-1 == magnitude).
   */
  int componentNumber() const;

  /**
   * Returns the view proxy corresponding to the set representation, if any.
   */
  vtkSMViewProxy* viewProxy() const;

  /**
   * Updates the scalar bar visibility of the representation `reprProxy` in `view`.
   * The behavior of the scalar bar visibility is dependent of the general settings.
   */
  static void updateScalarBarVisibility(vtkSMViewProxy* view, vtkSMProxy* reprProxy);

  /**
   * Returns the selected representation type as a string.
   */
  QString representationText() const { return this->RepresentationText; }

Q_SIGNALS:
  /**
   * fired to indicate the array-name changed.
   */
  void arraySelectionChanged();

  /**
   * fired to indicate the representation type changed, allowing us to update.
   */
  void representationTextChanged(const QString& text);

public Q_SLOTS:
  /**
   * Set the representation to control the scalar coloring properties on.
   */
  void setRepresentation(pqDataRepresentation* display);

  /**
   * set representation type.
   */
  void setRepresentationText(const QString& text);

private Q_SLOTS:
  /**
   * fills up the Variables combo-box using the active representation's
   * ColorArrayName property's domain.
   */
  void refreshColorArrayNames();

  /**
   * renders the view associated with the active representation.
   */
  void renderActiveView();

  /**
   * refresh the components combo-box.
   */
  void refreshComponents();

  /**
   * Called whenever the representation's color transfer function is changed.
   * We need the CTF for component selection. This has the side effect of
   * updating the component number UI to select the component used by the CTF.
   */
  void updateColorTransferFunction();

  /**
   * called when the UI for component number changes. We update the component
   * selection on this->ColorTransferFunction, if present.
   */
  void componentNumberChanged();

  /**
   * called when this->Variables is changed. If we added an entry to the
   * Variables combobox for an array not in the domain, then if it's not longer
   * needed, we prune that value.
   */
  void pruneOutOfDomainEntries();

protected:
  /**
   * Update the UI to show the selected array.
   */
  void setArraySelection(const ValueType&);
  /**
   * Update the UI to show the selected component.
   */
  void setComponentNumber(int);

private:
  QVariant itemData(int association, const QString& arrayName) const;
  QIcon* itemIcon(int association, const QString& arrayName) const;

  /**
   * called to add an out-of-domain value to the combo-box. Such a value is
   * needed when the SM property has a value which is no longer present in the
   * domain. In such a case, we still need the UI to show that value. But as
   * soon as the UI selection changes, we prune that value so the user cannot
   * select it again. Also, such a value is marked with a "(?)" suffix.
   * Returns the index for the newly added entry.
   */
  int addOutOfDomainEntry(int association, const QString& arrayName);

  QIcon* CellDataIcon;
  QIcon* PointDataIcon;
  QIcon* FieldDataIcon;
  QIcon* SolidColorIcon;
  QComboBox* Variables;
  QComboBox* Components;
  QPointer<pqDataRepresentation> Representation;
  QPointer<pqScalarsToColors> ColorTransferFunction;
  QString RepresentationText;

  // This is maintained to detect when the representation has changed.
  void* CachedRepresentation;

  class pqInternals;
  pqInternals* Internals;

  class PropertyLinksConnection;
  friend class PropertyLinksConnection;
};
#endif
