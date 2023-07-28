// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqColorOverlay_h
#define pqColorOverlay_h

#include "pqComponentsModule.h"
#include <QWidget>

/**
 * pqColorOverlay defines a widget with a uniform color and an opacity.
 * The color and the opacity are exposed as properties so they can
 *  be animated with a QPropertyAnimation
 */
class PQCOMPONENTS_EXPORT pqColorOverlay : public QWidget
{
  Q_OBJECT
  Q_PROPERTY(QColor rgb READ rgb WRITE setRgb)
  Q_PROPERTY(int opacity READ opacity WRITE setOpacity)

public:
  pqColorOverlay(QWidget* parent = nullptr);

  ///@{
  /**
   * Get/Set the red, green and blue values of the overlay
   * Defaults to white (255, 255, 255)
   */
  QColor rgb() const;
  void setRgb(int r, int g, int b);
  void setRgb(QColor Rgb);
  ///@}

  ///@{
  /**
   * Get/Set the opacity of the overlay
   * Default to 255
   */
  int opacity() const;
  void setOpacity(int opacity);
  ///@}

protected:
  void paintEvent(QPaintEvent*) override;

private:
  QColor Rgba = Qt::white;
};

#endif
