// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqTimelinePainter.h"

#include "pqCoreUtilities.h"
#include "pqTimelineModel.h"

#include "vtkMathUtilities.h"

#include <QModelIndex>
#include <QPainter>
#include <QStandardItem>
#include <QStyleOptionViewItem>

struct pqTimelinePainter::pqInternals
{
  // list displayed rectangles, to manage collisions.
  QList<QRect> LabelRects;
  QRect StartLabelRect;
  QRect EndLabelRect;

  QStyleOptionViewItem LastSceneTimeOption;

  // define several brushes based on palette
  QBrush backgroundBrush(const QStyleOptionViewItem& option, bool alternate = false)
  {
    return alternate ? option.palette.alternateBase() : option.palette.window();
  }
  QBrush sceneTimeBrush(const QStyleOptionViewItem& option) { return option.palette.link(); }
  QBrush sourceTimeBrush(const QStyleOptionViewItem& option)
  {
    return option.palette.linkVisited();
  }
  QBrush tickAndLabelBrush(const QStyleOptionViewItem& option) { return option.palette.base(); }
  QBrush lockedTimeBrush(const QStyleOptionViewItem& option) { return option.palette.highlight(); }
  QBrush editableTimeBrush(const QStyleOptionViewItem& option) { return option.palette.dark(); }
};

//-----------------------------------------------------------------------------
pqTimelinePainter::pqTimelinePainter(QObject* parent)
  : Superclass(parent)
  , Internals(new pqInternals())
{
}

//-----------------------------------------------------------------------------
pqTimelinePainter::~pqTimelinePainter() = default;

//-----------------------------------------------------------------------------
void pqTimelinePainter::paint(
  QPainter* painter, const QModelIndex& index, const QStyleOptionViewItem& option)
{
  auto model = dynamic_cast<const QStandardItemModel*>(index.model());
  auto item = model->itemFromIndex(index);

  switch (item->data().toInt())
  {
    case pqTimelineTrack::TIME:
    {
      this->paintBackground(painter, option, false);
      this->paintTimeTrack(painter, option, item);
      break;
    }
    case pqTimelineTrack::SOURCE:
    {
      this->paintBackground(painter, option, index.row() % 2 == 0);
      this->paintSourceTrack(painter, option, item);
      break;
    }
    case pqTimelineTrack::ANIMATION:
    {
      this->paintBackground(painter, option, index.row() % 2 == 0);
      this->paintAnimationTrack(painter, option, item);
      break;
    }
    default:
      break;
  }
}

//-----------------------------------------------------------------------------
void pqTimelinePainter::paintBackground(
  QPainter* painter, const QStyleOptionViewItem& option, bool alternate)
{
  painter->save();
  painter->setBrush(this->Internals->backgroundBrush(option, alternate));
  painter->setPen(Qt::NoPen);
  painter->drawRect(option.rect);
  painter->restore();
}

//-----------------------------------------------------------------------------
void pqTimelinePainter::paintTimeTrack(
  QPainter* painter, const QStyleOptionViewItem& option, QStandardItem* item)
{
  painter->save();
  painter->setBrush(this->Internals->backgroundBrush(option));

  // TODO: handle zoom
  // TODO: handle mouse event

  QStyleOptionViewItem sourceOption = option;
  int height = sourceOption.rect.height();
  sourceOption.rect.adjust(0, 0.6 * height, 0, 0);

  // draw labels on the top half
  QStyleOptionViewItem labelOption = option;
  labelOption.rect.adjust(0, 0, 0, -0.5 * height);

  // invalidate cache
  this->Internals->LabelRects.clear();
  this->Internals->StartLabelRect = QRect();
  this->Internals->EndLabelRect = QRect();

  // start and end time have specific painting. Handle them first.
  painter->save();
  // setup a background
  QStyleOptionViewItem startOption = labelOption;
  startOption.backgroundBrush = this->SceneLockStart
    ? this->Internals->lockedTimeBrush(labelOption)
    : this->Internals->editableTimeBrush(labelOption);
  if (this->paintTick(painter, option, item, this->SceneStartTime, true, startOption,
        pqCoreUtilities::formatTime(this->SceneStartTime)))
  {
    this->Internals->StartLabelRect = this->Internals->LabelRects.last();
  }

  QStyleOptionViewItem endOption = labelOption;
  endOption.backgroundBrush = this->SceneLockEnd ? this->Internals->lockedTimeBrush(labelOption)
                                                 : this->Internals->editableTimeBrush(labelOption);
  if (this->paintTick(painter, option, item, this->SceneEndTime, true, endOption,
        pqCoreUtilities::formatTime(this->SceneEndTime)))
  {
    this->Internals->EndLabelRect = this->Internals->LabelRects.last();
  }
  painter->restore();

  this->paintTimeline(painter, sourceOption, item, true, labelOption);

  this->paintSceneCurrentTime(painter, sourceOption);

  painter->restore();
}

//-----------------------------------------------------------------------------
void pqTimelinePainter::paintSourceTrack(
  QPainter* painter, const QStyleOptionViewItem& option, QStandardItem* item)
{
  QRectF timelineRect = option.rect;

  // TODO: handle zoom
  // TODO: handle mouse event

  painter->save();
  painter->setBrush(this->Internals->backgroundBrush(option));

  // add an horizontal middle line
  QLineF horzLine(timelineRect.left(), timelineRect.top() + timelineRect.height() / 2,
    timelineRect.left() + timelineRect.width(), timelineRect.top() + timelineRect.height() / 2);
  painter->drawLine(horzLine);

  QStyleOptionViewItem sourceOption = option;
  int height = sourceOption.rect.height();
  sourceOption.rect.adjust(0, 0.2 * height, 0, -0.2 * height);
  QStyleOptionViewItem unusedStyle;
  this->paintTimeline(painter, sourceOption, item, false, unusedStyle);

  this->paintSourcePipelineTime(painter, option, item);

  this->paintSceneCurrentTime(painter, option);

  painter->restore();
}

//-----------------------------------------------------------------------------
void pqTimelinePainter::paintAnimationTrack(
  QPainter* painter, const QStyleOptionViewItem& option, QStandardItem* item)
{
  painter->save();
  painter->setBrush(this->Internals->backgroundBrush(option));

  // TODO: handle zoom
  // TODO: handle mouse event

  // draw labels on the top half
  QStyleOptionViewItem animOption = option;
  int height = animOption.rect.height();
  animOption.rect.adjust(0, 0.6 * height, 0, 0);
  QStyleOptionViewItem labelOption = option;
  labelOption.rect.adjust(0, 0, 0, -0.3 * height);
  this->paintTimeline(painter, animOption, item, true, labelOption);

  this->paintSceneCurrentTime(painter, option);

  painter->restore();
}

// Paint time annotations. Return true if the annotation is effectively painted.
// Do not add label that collides on previously added labels.
bool pqTimelinePainter::paintLabel(QPainter* painter, const QStyleOptionViewItem& option,
  QStandardItem* item, double time, const QString& label)
{
  auto metrics = painter->fontMetrics();
  int labelSize = metrics.horizontalAdvance(label);

  // add some padding on left and right side of the label for readability.
  auto margin = option.rect.height();
  labelSize += margin / 2;

  int tickPos = this->positionFromTime(time, option);
  int pos = tickPos - labelSize / 2;

  // ensure label is fully inside timeline.
  pos = std::min(pos + labelSize, option.rect.right()) - labelSize;
  pos = std::max(pos, option.rect.left());

  QRect labelRect(pos, option.rect.top(), labelSize, option.rect.height());

  // manage collision for time track only. Handling collison for animation values labels
  // maybe done but requires a few refactor
  if (this->isTimeTrack(item))
  {
    // add more margins for collision detection, to get a smaller density of labels.
    auto rectWithMargins = labelRect.marginsAdded(QMargins(margin, 0, margin, 0));
    for (const auto& rect : this->Internals->LabelRects)
    {
      if (rectWithMargins.intersects(rect))
      {
        return false;
      }
    }
  }

  painter->fillRect(labelRect, option.backgroundBrush);

  this->Internals->LabelRects.append(labelRect);
  painter->drawText(labelRect, label);

  return true;
}

//-----------------------------------------------------------------------------
bool pqTimelinePainter::paintTick(QPainter* painter, const QStyleOptionViewItem& option,
  QStandardItem* item, double time, bool paintLabels, const QStyleOptionViewItem& labelsOption,
  const QString& label)
{
  double tickPos = this->positionFromTime(time, option);
  if (time < this->DisplayStartTime || time > this->DisplayEndTime)
  {
    // outside painting area, return.
    return false;
  }

  bool painted = false;
  QRect timelineRect = option.rect;
  int height = timelineRect.height();
  if (paintLabels)
  {
    painted = this->paintLabel(painter, labelsOption, item, time, label);
    if (!painted)
    {
      // non labelized ticks are half size.
      height /= 2;
    }
  }

  // draw tick
  QLineF line(tickPos, timelineRect.bottom() - height, tickPos, timelineRect.bottom());
  painter->drawLine(line);

  return painted;
}

//-----------------------------------------------------------------------------
void pqTimelinePainter::paintTimeline(QPainter* painter, const QStyleOptionViewItem& option,
  QStandardItem* item, bool paintLabels, const QStyleOptionViewItem& labelsOption)
{
  std::vector<double> times = this->getTimes(item);

  // if not enough timesteps
  if (times.size() < 2)
  {
    // we always want to have at least 2 rects in the list.
    this->Internals->LabelRects << QRect();
    this->Internals->LabelRects << QRect();
    return;
  }

  // draw times
  painter->save();
  painter->setBrush(this->Internals->tickAndLabelBrush(option));
  for (unsigned int idx = 0; idx < times.size(); idx++)
  {
    QString label =
      this->isTimeTrack(item) ? pqCoreUtilities::formatTime(times[idx]) : this->getLabel(item, idx);
    this->paintTick(painter, option, item, times[idx], paintLabels, labelsOption, label);
  }
  painter->restore();
}

//-----------------------------------------------------------------------------
void pqTimelinePainter::paintTimeMark(
  QPainter* painter, const QStyleOptionViewItem& option, double pos)
{
  painter->save();
  auto pen = painter->pen();
  pen.setWidth(pen.width() * 2);
  pen.setBrush(painter->brush());
  painter->setPen(pen);

  double top = option.rect.top();
  double bottom = top + option.rect.height();
  QLineF mark(pos, top, pos, bottom);
  painter->drawLine(mark);
  painter->restore();
}

//-----------------------------------------------------------------------------
void pqTimelinePainter::paintSceneCurrentTime(QPainter* painter, const QStyleOptionViewItem& option)
{
  painter->save();
  painter->setBrush(this->Internals->sceneTimeBrush(option));
  double pos = this->positionFromTime(this->SceneCurrentTime, option);
  this->paintTimeMark(painter, option, pos);
  painter->restore();
  this->Internals->LastSceneTimeOption = option;
}

//-----------------------------------------------------------------------------
void pqTimelinePainter::paintSourcePipelineTime(
  QPainter* painter, const QStyleOptionViewItem& option, QStandardItem* item)
{
  painter->save();
  painter->setBrush(this->Internals->sourceTimeBrush(option));
  double pos = this->positionFromTime(this->getSourceTime(item), option);
  this->paintTimeMark(painter, option, pos);
  painter->restore();
}

//-----------------------------------------------------------------------------
QPair<double, double> pqTimelinePainter::displayTimeRange()
{
  return QPair<double, double>(this->DisplayStartTime, this->DisplayEndTime);
}

//-----------------------------------------------------------------------------
void pqTimelinePainter::setDisplayTimeRange(double start, double end)
{
  this->DisplayStartTime = start;
  this->DisplayEndTime = end;
}

//-----------------------------------------------------------------------------
double pqTimelinePainter::timeFromPosition(double pos, const QStyleOptionViewItem& option)
{
  double width = option.rect.width();
  return this->DisplayStartTime +
    (this->DisplayEndTime - this->DisplayStartTime) * (pos - option.rect.x()) / width;
}

//-----------------------------------------------------------------------------
double pqTimelinePainter::timeFromPosition(double pos)
{
  return this->timeFromPosition(pos, this->Internals->LastSceneTimeOption);
}

//-----------------------------------------------------------------------------
double pqTimelinePainter::indexTimeFromPosition(
  double pos, const QStyleOptionViewItem& option, const QModelIndex& index)
{
  auto model = dynamic_cast<const QStandardItemModel*>(index.model());
  auto item = model->itemFromIndex(index);

  double clickedTime = this->timeFromPosition(pos, option);

  auto times = this->getTimes(item);
  if (times.empty())
  {
    return clickedTime;
  }

  double time = times[0];
  for (auto sceneTime : times)
  {
    if (vtkMathUtilities::FuzzyCompare(clickedTime, sceneTime))
    {
      return clickedTime;
    }

    double tolerance = (sceneTime - time);
    if (sceneTime - tolerance / 2 > clickedTime)
    {
      return time;
    }

    time = sceneTime;
  }

  return time;
}

//-----------------------------------------------------------------------------
double pqTimelinePainter::positionFromTime(double time)
{
  return this->positionFromTime(time, this->Internals->LastSceneTimeOption);
}

//-----------------------------------------------------------------------------
double pqTimelinePainter::positionFromTime(double time, const QStyleOptionViewItem& option)
{
  double width = option.rect.width();
  return option.rect.x() +
    (time - this->DisplayStartTime) * width / (this->DisplayEndTime - this->DisplayStartTime);
}

//-----------------------------------------------------------------------------
bool pqTimelinePainter::isTimeTrack(QStandardItem* item)
{
  QVariant dataVariant = item->data(pqTimelineItemRole::TYPE);
  return dataVariant.toInt() == pqTimelineTrack::TIME;
}

//-----------------------------------------------------------------------------
bool pqTimelinePainter::isAnimationTrack(QStandardItem* item)
{
  QVariant dataVariant = item->data(pqTimelineItemRole::TYPE);
  return dataVariant.toInt() == pqTimelineTrack::ANIMATION;
}

//-----------------------------------------------------------------------------
std::vector<double> pqTimelinePainter::getTimes(QStandardItem* item)
{
  std::vector<double> times;
  QVariant dataVariant = item->data(pqTimelineItemRole::TIMES);
  for (auto time : dataVariant.toList())
  {
    double adjustedTime = time.toDouble();
    // animation keyframes store relative times.
    if (this->isAnimationTrack(item))
    {
      adjustedTime =
        this->SceneStartTime + adjustedTime * (this->SceneEndTime - this->SceneStartTime);
    }

    times.push_back(adjustedTime);
  }

  return times;
}

//-----------------------------------------------------------------------------
double pqTimelinePainter::getSourceTime(QStandardItem* item)
{
  QVariant dataVariant = item->data(pqTimelineItemRole::SOURCE_TIME);
  return dataVariant.toDouble();
}

//-----------------------------------------------------------------------------
QString pqTimelinePainter::getLabel(QStandardItem* item, int index)
{
  QVariant dataVariant = item->data(pqTimelineItemRole::LABELS);
  QVariantList values = dataVariant.toList();
  if (values.size() > index)
  {
    return pqCoreUtilities::formatNumber(values[index].toDouble());
  }

  return QString("");
}

//-----------------------------------------------------------------------------
void pqTimelinePainter::setSceneStartTime(double time)
{
  this->SceneStartTime = time;
}

//-----------------------------------------------------------------------------
void pqTimelinePainter::setSceneEndTime(double time)
{
  this->SceneEndTime = time;
}

//-----------------------------------------------------------------------------
void pqTimelinePainter::setSceneCurrentTime(double time)
{
  this->SceneCurrentTime = time;
}

//-----------------------------------------------------------------------------
double pqTimelinePainter::getSceneCurrentTime()
{
  return this->SceneCurrentTime;
}

//-----------------------------------------------------------------------------
void pqTimelinePainter::setSceneLockStart(bool lock)
{
  this->SceneLockStart = lock;
}

//-----------------------------------------------------------------------------
void pqTimelinePainter::setSceneLockEnd(bool lock)
{
  this->SceneLockEnd = lock;
}

//-----------------------------------------------------------------------------
bool pqTimelinePainter::hasStartEndLabels()
{
  return this->Internals->StartLabelRect.isValid() || this->Internals->EndLabelRect.isValid();
}

//-----------------------------------------------------------------------------
QRect pqTimelinePainter::getStartLabelRect()
{
  return this->Internals->StartLabelRect;
}

//-----------------------------------------------------------------------------
QRect pqTimelinePainter::getEndLabelRect()
{
  return this->Internals->EndLabelRect;
}
