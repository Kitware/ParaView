// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqBoxPropertyWidget_h
#define pqBoxPropertyWidget_h

#include "pqInteractivePropertyWidget.h"
#include <QPointer> // For QScopedPointer

/**
 * @class pqBoxPropertyWidget
 * @brief custom property widget using vtkBoxWidget2 and vtkBoxRepresentation.
 *
 * pqBoxPropertyWidget is a custom property widget that uses
 * "BoxWidgetRepresentation" to help users interactively set the origin,
 * orientation and scale for an oriented bounding box.
 *
 * To use this widget for a property group (`vtkSMPropertyGroup`), use "InteractiveBox"
 * as the "panel_widget" in the XML configuration for the proxy. The property group should
 * have properties for following functions (all of which are optional):
 *
 * * **Position**: a 3-tuple vtkSMDoubleVectorProperty that will be linked to the
 * translation/position of the box.
 * * **Rotation**: a 3-tuple vtkSMDoubleVectorProperty that will be linked to the
 * orientation for the box.
 * * **Scale** : a 3-tuple vtkSMDoubleVectorProperty that will be linked to the
 * scale for the box.
 * * **Input**: a vtkSMInputProperty that is used to get data information for bounds
 * when placing/resetting the widget.
 * * **UseReferenceBounds**: a vtkSMIntVectorProperty that enables the widget to
 * place the box relative to unit box or a explicitly specified bounds.
 * * **ReferenceBounds**: a vtkSMDoubleVectorProperty with 6 elements that is linked
 * to bounds used to place the box widget
 *
 * **UseReferenceBounds** and **ReferenceBounds** must be used together i.e. both
 * are required for this to work.
 *
 * The constructor accepts a boolean "hideReferenceBounds" to toggle the visibility
 * of reference bounds items. This is useful when reference bounds are fixed by an
 * external entity that doesn't want the user to modify it.
 *
 * Note while all of the above are optional, it really doesn't make much sense
 * to use this widget if any of them are not specified.
 */

class PQAPPLICATIONCOMPONENTS_EXPORT pqBoxPropertyWidget : public pqInteractivePropertyWidget
{
  Q_OBJECT
  typedef pqInteractivePropertyWidget Superclass;

public:
  pqBoxPropertyWidget(vtkSMProxy* proxy, vtkSMPropertyGroup* smgroup, QWidget* parent = nullptr,
    bool hideReferenceBounds = false);
  ~pqBoxPropertyWidget() override;

protected Q_SLOTS:
  /**
   * Places the interactive widget using current data source information.
   */
  void placeWidget() override;

private:
  Q_DISABLE_COPY(pqBoxPropertyWidget)
  class pqUi;
  QScopedPointer<pqUi> Ui;
  pqPropertyLinks WidgetLinks;
  bool BoxIsRelativeToInput;

  QMetaObject::Connection PlaceWidgetConnection;
};

#endif
