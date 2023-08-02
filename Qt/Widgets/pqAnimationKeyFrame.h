// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqAnimationKeyFrame_h
#define pqAnimationKeyFrame_h

#include "pqWidgetsModule.h"

#include "vtkParaViewDeprecation.h"

#include <QGraphicsItem>
#include <QIcon>
#include <QObject>
class pqAnimationTrack;

// represents a key frame
class PARAVIEW_DEPRECATED_IN_5_12_0(
  "See `pqTimeManagerWidget` for new design") PQWIDGETS_EXPORT pqAnimationKeyFrame
  : public QObject
  , public QGraphicsItem
{
  Q_OBJECT
  /**
   * the time as a fraction of scene time that this keyframe starts at
   */
  Q_PROPERTY(double normalizedStartTime READ normalizedStartTime WRITE setNormalizedStartTime)
  /**
   * the time as a fraction of scene time that this keyframe ends at
   */
  Q_PROPERTY(double normalizedEndTime READ normalizedEndTime WRITE setNormalizedEndTime)
  /**
   * the value at the start of the keyframe
   */
  Q_PROPERTY(QVariant startValue READ startValue WRITE setStartValue)
  /**
   * the value at the end of the keyframe
   */
  Q_PROPERTY(QVariant endValue READ endValue WRITE setEndValue)
  /**
   * an icon to help describe the keyframe
   */
  Q_PROPERTY(QIcon icon READ icon WRITE setIcon)
  Q_INTERFACES(QGraphicsItem)
public:
  pqAnimationKeyFrame(pqAnimationTrack* p);
  ~pqAnimationKeyFrame() override = default;

  double normalizedStartTime() const;
  double normalizedEndTime() const;
  QVariant startValue() const;
  QVariant endValue() const;
  QIcon icon() const;

  QRectF boundingRect() const override;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  void setNormalizedStartTime(double t);
  void setNormalizedEndTime(double t);
  void setStartValue(const QVariant&);
  void setEndValue(const QVariant&);
  void setIcon(const QIcon& icon);
  void setBoundingRect(const QRectF& r);
  void adjustRect(double startPos, double endPos);

Q_SIGNALS:
  void startValueChanged();
  void endValueChanged();
  void iconChanged();

protected:
  /**
   * Returns the parent pqAnimationTrack.
   */
  pqAnimationTrack* parentTrack() const;

  void paint(QPainter* p, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
  double NormalizedStartTime = 0;
  double NormalizedEndTime = 1;
  QVariant StartValue;
  QVariant EndValue;
  QIcon Icon;

  QRectF Rect;
};

#endif // pqAnimationKeyFrame_h
