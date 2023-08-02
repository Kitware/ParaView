// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqFontPropertyWidget_h
#define pqFontPropertyWidget_h

#include "pqApplicationComponentsModule.h"
#include "pqPropertyGroupWidget.h"

class QActionGroup;
class vtkSMPropertyGroup;

/**
 * pqFontPropertyWidget is a pqPropertyWidget that can be used to set
 * properties relating to fonts. The widget expects the property-group to have
 * properties with functions set to "Color", "Opacity", "Family", "Size",
 * "Bold", "Italics" and "Shadow". If any property is missing, the
 * corresponding widget will be hidden.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqFontPropertyWidget : public pqPropertyGroupWidget
{
  Q_OBJECT
  Q_PROPERTY(
    QString HorizontalJustification READ HorizontalJustification WRITE setHorizontalJustification)
  Q_PROPERTY(
    QString VerticalJustification READ VerticalJustification WRITE setVerticalJustification)

  typedef pqPropertyGroupWidget Superclass;

public:
  pqFontPropertyWidget(vtkSMProxy* proxy, vtkSMPropertyGroup* smgroup, QWidget* parent = nullptr);
  ~pqFontPropertyWidget() override;

  QString HorizontalJustification() const;
  QString VerticalJustification() const;
Q_SIGNALS:
  void horizontalJustificationChanged(QString&);
  void verticalJustificationChanged(QString&);

protected:
  void setHorizontalJustification(QString&);
  void setVerticalJustification(QString&);
  void setupHorizontalJustificationButton();
  void setupVerticalJustificationButton();
  void UpdateToolButtonIcon(QString& str, QToolButton* justification);
  QActionGroup* CreateFontActionGroup(QToolButton* justification);

protected Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  void changeHorizontalJustificationIcon(QAction*);
  void changeVerticalJustificationIcon(QAction*);
  void onFontFamilyChanged();

private:
  Q_DISABLE_COPY(pqFontPropertyWidget)

  class pqInternals;
  pqInternals* Internals;
};

#endif
