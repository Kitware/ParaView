/*=========================================================================

   Program: ParaView
   Module:  pqDisplayColorWidget.h

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#ifndef _pqDisplayColorWidget_h
#define _pqDisplayColorWidget_h

#include "pqComponentsModule.h"

#include <QPair>
#include <QPointer>
#include <QWidget>

class pqDataRepresentation;
class pqScalarsToColors;
class QComboBox;
class vtkEventQtSlotConnect;
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
  typedef QWidget Superclass;

public:
  typedef QPair<int, QString> ValueType;

public:
  pqDisplayColorWidget(QWidget* parent = 0);
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

Q_SIGNALS:
  /**
  * fired to indicate the array-name changed.
  */
  void arraySelectionChanged();

public Q_SLOTS:
  /**
  * Set the representation to control the scalar coloring properties on.
  */
  void setRepresentation(pqDataRepresentation* display);

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

private:
  QIcon* CellDataIcon;
  QIcon* PointDataIcon;
  QIcon* FieldDataIcon;
  QIcon* SolidColorIcon;
  QComboBox* Variables;
  QComboBox* Components;
  QPointer<pqDataRepresentation> Representation;
  QPointer<pqScalarsToColors> ColorTransferFunction;

  // This is maintained to detect when the representation has changed.
  void* CachedRepresentation;

  class pqInternals;
  pqInternals* Internals;

  class PropertyLinksConnection;
  friend class PropertyLinksConnection;
};
#endif
