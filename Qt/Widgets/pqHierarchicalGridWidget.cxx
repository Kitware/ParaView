// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqHierarchicalGridWidget.h"

#include "pqHierarchicalGridLayout.h"

#include <QApplication>
#include <QMouseEvent>
#include <QRect>
#include <QRegion>
#include <QRubberBand>

#include <cassert>
#include <utility>
#include <vector>

//-----------------------------------------------------------------------------
class pqHierarchicalGridWidget::pqInternals
{
  int Active = -1;
  int MouseOffset = 0;
  bool CursorOverridden = false;

public:
  bool UserResizability = true;
  QRegion HSplitRegion;
  QRegion VSplitRegion;
  QScopedPointer<QRubberBand> RubberBand;

  std::vector<std::pair<pqHierarchicalGridWidget::SplitterInfo, QRect>> Splitters;

  bool isMoving() const { return this->Active != -1; }
  void start(const QPoint& pos)
  {
    this->Active = -1;
    this->MouseOffset = 0;
    int index = 0;
    for (const auto& spair : this->Splitters)
    {
      if (spair.second.contains(pos))
      {
        this->Active = index;
        this->MouseOffset = (spair.first.Direction == Qt::Horizontal ? pos.x() : pos.y());
        break;
      }
      ++index;
    }
  }
  void stop(const QPoint& pos, pqHierarchicalGridWidget* self)
  {
    if (this->RubberBand->isVisible() == false)
    {
      // this happens when user click without moving mouse; in that case,
      // do nothing.
      this->Active = -1;
      this->MouseOffset = 0;
    }
    else
    {
      this->move(pos);
      this->RubberBand->hide();
      assert(this->Active != -1 && static_cast<int>(this->Splitters.size()) > this->Active);

      const auto& spair = this->Splitters[this->Active];
      const auto& point = this->RubberBand->geometry().center();
      double fraction = (spair.first.Direction == Qt::Horizontal)
        ? static_cast<double>(point.x() - spair.first.Bounds.left()) / spair.first.Bounds.width()
        : static_cast<double>(point.y() - spair.first.Bounds.top()) / spair.first.Bounds.height();
      this->Active = -1;
      this->MouseOffset = 0;

      self->setSplitFraction(spair.first.Location, fraction);
    }
  }

  void move(const QPoint& pos)
  {
    assert(this->Active != -1 && static_cast<int>(this->Splitters.size()) > this->Active);

    const auto& spair = this->Splitters[this->Active];
    int newpos = (spair.first.Direction == Qt::Horizontal ? pos.x() : pos.y());

    // now clamp to splitter bounds.

    QRect r = spair.second;
    if (spair.first.Direction == Qt::Horizontal)
    {
      newpos = std::min(newpos, spair.first.Bounds.right());
      newpos = std::max(newpos, spair.first.Bounds.left());
      r.moveLeft(newpos);
    }
    else
    {
      newpos = std::min(newpos, spair.first.Bounds.bottom());
      newpos = std::max(newpos, spair.first.Bounds.top());
      r.moveTop(newpos);
    }

    this->RubberBand->setGeometry(r);
    this->RubberBand->show();
  }

  void setCursor(const QCursor& cursor, QWidget* self)
  {
    this->CursorOverridden = true;
    self->setCursor(cursor);
  }

  void restoreCursor(QWidget* self)
  {
    if (this->CursorOverridden)
    {
      this->CursorOverridden = false;
      self->unsetCursor();
    }
  }
};

//-----------------------------------------------------------------------------
pqHierarchicalGridWidget::pqHierarchicalGridWidget(QWidget* parentObject)
  : Superclass(parentObject)
  , Internals(new pqHierarchicalGridWidget::pqInternals())
{
  auto& internals = (*this->Internals);
  internals.RubberBand.reset(new QRubberBand(QRubberBand::Line, this));
  this->setMouseTracking(true);

  // needed to ensure we get all mouse move events so we can correctly restore
  // cursor when the mouse moves out of the spacers and onto one of our nested
  // widgets.
  qApp->installEventFilter(this);
}

//-----------------------------------------------------------------------------
pqHierarchicalGridWidget::~pqHierarchicalGridWidget() = default;

//-----------------------------------------------------------------------------
void pqHierarchicalGridWidget::setUserResizability(bool val)
{
  auto& internals = (*this->Internals);
  internals.UserResizability = val;
}

//-----------------------------------------------------------------------------
bool pqHierarchicalGridWidget::userResizability() const
{
  auto& internals = (*this->Internals);
  return internals.UserResizability;
}

//-----------------------------------------------------------------------------
void pqHierarchicalGridWidget::setSplitters(
  const QVector<pqHierarchicalGridWidget::SplitterInfo>& splitters)
{
  auto& internals = (*this->Internals);
  internals.HSplitRegion = QRegion();
  internals.VSplitRegion = QRegion();
  internals.Splitters.clear();

  for (const auto& s : splitters)
  {
    if (s.Direction == Qt::Horizontal)
    {
      QRect r(s.Bounds.x() + s.Position - 2, s.Bounds.y(), 4, s.Bounds.height());
      internals.HSplitRegion = internals.HSplitRegion.united(r);
      internals.Splitters.push_back(std::make_pair(s, r));
    }
    else
    {
      QRect r(s.Bounds.x(), s.Bounds.y() + s.Position - 2, s.Bounds.width(), 4);
      internals.VSplitRegion = internals.VSplitRegion.united(r);
      internals.Splitters.push_back(std::make_pair(s, r));
    }
  }
}

//-----------------------------------------------------------------------------
bool pqHierarchicalGridWidget::eventFilter(QObject* caller, QEvent* evt)
{
  auto& internals = (*this->Internals);
  if (evt->type() == QEvent::MouseMove && !internals.isMoving())
  {
    // this is needed since `mouseMoveEvent` is not called when the mouse moves
    // over one of our children; we need to make sure the resize cursor
    // doesn't linger when mouse is over the child widgets.
    internals.restoreCursor(this);
  }
  return this->Superclass::eventFilter(caller, evt);
}

//-----------------------------------------------------------------------------
void pqHierarchicalGridWidget::mouseMoveEvent(QMouseEvent* evt)
{
  // fixme: add global event filter to correctly handle cursor.
  auto& internals = (*this->Internals);
  if (!internals.UserResizability)
  {
    return this->Superclass::mouseMoveEvent(evt);
  }

  if (internals.isMoving())
  {
    internals.move(evt->pos());
  }
  else if (internals.HSplitRegion.contains(evt->pos()))
  {
    internals.setCursor(Qt::SplitHCursor, this);
  }
  else if (internals.VSplitRegion.contains(evt->pos()))
  {
    internals.setCursor(Qt::SplitVCursor, this);
  }
  else
  {
    internals.restoreCursor(this);
  }
}

//-----------------------------------------------------------------------------
void pqHierarchicalGridWidget::mousePressEvent(QMouseEvent* evt)
{
  auto& internals = (*this->Internals);
  if (!internals.UserResizability)
  {
    return this->Superclass::mouseMoveEvent(evt);
  }

  if (evt->button() == Qt::LeftButton)
  {
    internals.start(evt->pos());
  }
  else
  {
    this->Superclass::mousePressEvent(evt);
  }
}

//-----------------------------------------------------------------------------
void pqHierarchicalGridWidget::mouseReleaseEvent(QMouseEvent* evt)
{
  auto& internals = (*this->Internals);
  if (evt->button() == Qt::LeftButton && internals.isMoving() && internals.UserResizability)
  {
    internals.stop(evt->pos(), this);
  }
  else
  {
    this->Superclass::mouseReleaseEvent(evt);
  }
}

//-----------------------------------------------------------------------------
void pqHierarchicalGridWidget::setSplitFraction(int location, double fraction)
{
  if (auto l = qobject_cast<pqHierarchicalGridLayout*>(this->layout()))
  {
    l->setSplitFraction(location, fraction);
  }
  Q_EMIT this->splitterMoved(location, fraction);
}
