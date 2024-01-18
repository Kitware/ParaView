// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqXYChartViewBoundsPropertyWidget_h
#define pqXYChartViewBoundsPropertyWidget_h

#include "pqComponentsModule.h"
#include "pqPropertyWidget.h"

#include <QScopedPointer>

/**
 * @class pqXYChartViewBoundsPropertyWidget
 * @brief Extract bottom left axis range into a property
 *
 * A property widget that extract bottom and left axis range
 * from a pqXYChartView if it is the current active view into
 * a 4 component double vector property.
 *
 * A reset button is added in order to actually perform the
 * extraction.
 *
 */
class PQCOMPONENTS_EXPORT pqXYChartViewBoundsPropertyWidget : public pqPropertyWidget
{
  Q_OBJECT
  Q_PROPERTY(QList<QVariant> bounds READ bounds WRITE setBounds NOTIFY boundsChanged);
  typedef pqPropertyWidget Superclass;

public:
  explicit pqXYChartViewBoundsPropertyWidget(
    vtkSMProxy* proxy, vtkSMProperty* property, QWidget* parent = nullptr);
  ~pqXYChartViewBoundsPropertyWidget() override;

  ///@{
  /**
   * Get/Set the chart bounds. Size should always be 4.
   */
  QList<QVariant> bounds();
  void setBounds(const QList<QVariant>& bounds);
  ///@}

Q_SIGNALS:
  /**
   * Signal fired whenever the bounds are changed.
   */
  void boundsChanged();

protected Q_SLOTS:
  /**
   * Extract bounds from the current pqXYChartView if any
   */
  void resetBounds();

  /**
   * Called when the text is changed in order to update
   * the internally stored bounds
   */
  void onTextChanged();

  /**
   * Called when the bounds are changed externally to update
   * the texts
   */
  void updateTextFromBounds();

  /**
   * Initialize the bounds on the first show
   */
  void showEvent(QShowEvent* event) override;

  /**
   * Called whenever the active view change
   * Keep track of the active view and connect
   * to pqXYChartView axis range property to manage
   * the reset button
   */
  void connectToView(pqView* view);

private:
  Q_DISABLE_COPY(pqXYChartViewBoundsPropertyWidget)

  struct pqInternals;
  QScopedPointer<pqInternals> Internals;
};

#endif
