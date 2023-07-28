// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqAnimationTrack_h
#define pqAnimationTrack_h

#include "pqWidgetsModule.h"

#include "vtkParaViewDeprecation.h"

#include <QGraphicsItem>
#include <QList>
#include <QObject>

class pqAnimationKeyFrame;

// represents a track
class PARAVIEW_DEPRECATED_IN_5_12_0(
  "See `pqTimeManagerWidget` for new design. See also pqTimelineItemDelegate")
  PQWIDGETS_EXPORT pqAnimationTrack
  : public QObject
  , public QGraphicsItem
{
  Q_OBJECT
  Q_INTERFACES(QGraphicsItem)
  /**
   * the property animated in this track
   */
  Q_PROPERTY(QVariant property READ property WRITE setProperty)
public:
  pqAnimationTrack(QObject* p = nullptr);
  ~pqAnimationTrack() override;

  /**
   * number of keyframes
   */
  int count();
  /**
   * get a keyframe
   */
  pqAnimationKeyFrame* keyFrame(int);

  /**
   * add a keyframe
   */
  pqAnimationKeyFrame* addKeyFrame();
  /**
   * remove a keyframe
   */
  void removeKeyFrame(pqAnimationKeyFrame* frame);

  bool isDeletable() const { return this->Deletable; }
  void setDeletable(bool d) { this->Deletable = d; }

  QVariant property() const;

  QRectF boundingRect() const override;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  void setProperty(const QVariant& p);

  void setBoundingRect(const QRectF& r);

  void setEnabled(bool enable)
  {
    this->QGraphicsItem::setEnabled(enable);
    Q_EMIT this->enabledChanged();
  }

Q_SIGNALS:
  void propertyChanged();
  void enabledChanged();

protected:
  void paint(QPainter* p, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
  bool Deletable = true;
  QList<pqAnimationKeyFrame*> Frames;
  QVariant Property;

  QRectF Rect;
};

#endif // pqAnimationTrack_h
