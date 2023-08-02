// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqDisplayRepresentationWidget_h
#define pqDisplayRepresentationWidget_h

#include "pqComponentsModule.h"
#include "pqPropertyWidget.h"
#include <QWidget>

class pqDataRepresentation;
class vtkSMProxy;
class vtkSMViewProxy;

/**
 * A widget for representation type for a vtkSMRepresentationProxy. It works
 * with a vtkSMRepresentationProxy, calling
 * vtkSMRepresentationProxy::SetRepresentationType() to change the
 * representation type to the one chosen by the user.
 */
class PQCOMPONENTS_EXPORT pqDisplayRepresentationWidget : public QWidget
{
  Q_OBJECT;
  Q_PROPERTY(QString representationText READ representationText WRITE setRepresentationText NOTIFY
      representationTextChanged);
  typedef QWidget Superclass;

public:
  pqDisplayRepresentationWidget(QWidget* parent = nullptr);
  ~pqDisplayRepresentationWidget() override;

  /**
   * Returns the selected representation as a string.
   */
  QString representationText() const;

  /**
   * Returns the view proxy corresponding to the set representation, if any.
   */
  vtkSMViewProxy* viewProxy() const;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * set the representation proxy or pqDataRepresentation instance.
   */
  void setRepresentation(pqDataRepresentation* display);
  void setRepresentation(vtkSMProxy* proxy);

  /**
   * set representation type.
   */
  void setRepresentationText(const QString&);

private Q_SLOTS:
  /**
   * Slot called when the combo-box is changed. If this change was due to
   * a UI interaction, we need to prompt the user if he really intended to make
   * that change (Issue #15117).
   */
  void comboBoxChanged(const QString&);

Q_SIGNALS:
  void representationTextChanged(const QString&);

private:
  Q_DISABLE_COPY(pqDisplayRepresentationWidget)

  class pqInternals;
  pqInternals* Internal;

  class PropertyLinksConnection;

  pqDataRepresentation* Representation = nullptr;
};

/**
 * A property widget for selecting the display representation.
 */
class PQCOMPONENTS_EXPORT pqDisplayRepresentationPropertyWidget : public pqPropertyWidget
{
  Q_OBJECT

public:
  pqDisplayRepresentationPropertyWidget(vtkSMProxy* proxy, QWidget* parent = nullptr);
  ~pqDisplayRepresentationPropertyWidget() override;

private:
  pqDisplayRepresentationWidget* Widget;
};

#endif
