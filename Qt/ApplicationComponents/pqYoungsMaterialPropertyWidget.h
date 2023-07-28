// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqYoungsMaterialPropertyWidget_h
#define pqYoungsMaterialPropertyWidget_h

#include "pqApplicationComponentsModule.h"
#include "pqStringVectorPropertyWidget.h"

#include <QScopedPointer>

class vtkSMPropertyGroup;
class QStandardItem;

/**
 * This is a custom widget for YoungsMaterialInterface filter. We use a custom widget
 * since this filter has unusual requirements when it comes to setting
 * OrderingArrays and NormalArrays properties.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqYoungsMaterialPropertyWidget
  : public pqStringVectorPropertyWidget
{
  Q_OBJECT
  typedef pqStringVectorPropertyWidget Superclass;
  Q_PROPERTY(QList<QVariant> orderingArrays READ orderingArrays WRITE setOrderingArrays NOTIFY
      orderingArraysChanged);
  Q_PROPERTY(QList<QVariant> normalArrays READ normalArrays WRITE setNormalArrays NOTIFY
      normalArraysChanged);

public:
  pqYoungsMaterialPropertyWidget(
    vtkSMProxy* proxy, vtkSMPropertyGroup* group, QWidget* parent = nullptr);
  ~pqYoungsMaterialPropertyWidget() override;

  ///@{
  /**
   * Get/Set the ordering arrays, linked to the SMProperty
   */
  void setOrderingArrays(const QList<QVariant>&);
  QList<QVariant> orderingArrays() const;
  ///@}

  ///@{
  /**
   * Get/Set the normal arrays, linked to the SMProperty
   */
  void setNormalArrays(const QList<QVariant>&);
  QList<QVariant> normalArrays() const;
  ///@}

Q_SIGNALS:

  /**
   * Emitted when the ordering arrays change, linked ot the SMProperty
   */
  void orderingArraysChanged();

  /**
   * Emitted when the normal arrays change, linked ot the SMProperty
   */
  void normalArraysChanged();

protected:
  /**
   * Recover the currently selecticted item in the VolumeFractionArrays widget
   */
  QStandardItem* currentItem();

protected Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Called when the ordering array is changed
   */
  void onOrderingArraysChanged();

  /**
   * Called when the normal array is changed
   */
  void onNormalArraysChanged();

  /**
   * Called to initialize/update the combobox according to the current item
   */
  void updateComboBoxes();

private:
  Q_DISABLE_COPY(pqYoungsMaterialPropertyWidget)

  class pqInternals;
  QScopedPointer<pqInternals> Internals;
};

#endif
