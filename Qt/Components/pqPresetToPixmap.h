// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqPresetToPixmap_h
#define pqPresetToPixmap_h

#include "pqComponentsModule.h"
#include <QObject>
#include <QScopedPointer>
#include <vtk_jsoncpp_fwd.h> // for forward declarations

class vtkPiecewiseFunction;
class vtkScalarsToColors;
class QPixmap;
class QSize;

/**
 * pqPresetToPixmap is a helper class to generate QPixmap from a color/opacity
 * preset. Use pqPresetToPixmap::render() to generate a QPixmap for a transfer
 * function.
 */
class PQCOMPONENTS_EXPORT pqPresetToPixmap : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqPresetToPixmap(QObject* parent = nullptr);
  ~pqPresetToPixmap() override;

  /**
   * Render a preset to a pixmap for the given resolution.
   */
  QPixmap render(const Json::Value& preset, const QSize& resolution) const;

protected:
  /**
   * Renders a color transfer function preset.
   */
  QPixmap renderColorTransferFunction(
    vtkScalarsToColors* stc, vtkPiecewiseFunction* pf, const QSize& resolution) const;

  /**
   * Renders a color transfer function preset.
   */
  QPixmap renderIndexedColorTransferFunction(
    vtkScalarsToColors* stc, const QSize& resolution) const;

private:
  Q_DISABLE_COPY(pqPresetToPixmap)
  class pqInternals;
  QScopedPointer<pqInternals> Internals;
};

#endif
