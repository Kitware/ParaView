
#include "pqAnimationKeyFrame.h"

#include <QPainter>
#include <QFontMetrics>
#include <QWidget>
#include <QGraphicsView>
#include "pqAnimationTrack.h"

pqAnimationKeyFrame::pqAnimationKeyFrame(pqAnimationTrack* p, QGraphicsScene* s)
  : QObject(p), QGraphicsItem(p,s),
  StartTime(0), EndTime(1),
  Interpolation(Linear), Rect(0,0,1,1)
{
}

pqAnimationKeyFrame::~pqAnimationKeyFrame()
{
}

QVariant pqAnimationKeyFrame::startValue() const
{
  return this->StartValue;
}
QVariant pqAnimationKeyFrame::endValue() const
{
  return this->EndValue;
}

pqAnimationKeyFrame::InterpolationType 
pqAnimationKeyFrame::interpolation() const
{
  return this->Interpolation;
}

double pqAnimationKeyFrame::startTime() const
{
  return this->StartTime;
}
double pqAnimationKeyFrame::endTime() const
{
  return this->EndTime;
}
void pqAnimationKeyFrame::setStartTime(double t)
{
  this->StartTime = t;
  this->adjustRect();
}
void pqAnimationKeyFrame::setEndTime(double t)
{
  this->EndTime = t;
  this->adjustRect();
}

void pqAnimationKeyFrame::setStartValue(const QVariant& v)
{
  this->StartValue = v;
}
void pqAnimationKeyFrame::setEndValue(const QVariant& v)
{
  this->EndValue = v;
}
void pqAnimationKeyFrame::setInterpolation(
  pqAnimationKeyFrame::InterpolationType i)
{
  this->Interpolation = i;
}
  
QRectF pqAnimationKeyFrame::boundingRect() const
{ 
  return this->Rect;
}
  
void pqAnimationKeyFrame::setBoundingRect(const QRectF& r)
{
  this->removeFromIndex();
  this->Rect = r;
  this->addToIndex();
}

void pqAnimationKeyFrame::adjustRect()
{
  pqAnimationTrack* track = qobject_cast<pqAnimationTrack*>(this->parent());
  QRectF trackRect = track->boundingRect();;

  double w = trackRect.width();
  double totalTime = track->endTime() - track->startTime();

  double left = trackRect.left() + w * (this->startTime() - track->startTime()) / totalTime;
  double width = trackRect.width() * (this->endTime() - this->startTime()) / totalTime;

  this->setBoundingRect(QRectF(left, trackRect.top(), width, trackRect.height()));
}


void pqAnimationKeyFrame::paint(QPainter* p,
                   const QStyleOptionGraphicsItem *,
                   QWidget * widget)
{
  p->save();
  p->setBrush(QBrush(QColor(255,255,255)));
  QPen pen(QColor(0,0,0));
  pen.setWidth(0);
  p->setPen(pen);
  QRectF keyFrameRect(this->boundingRect());
  p->drawRect(keyFrameRect);

  QFontMetrics metrics(widget->font());
  double halfWidth = keyFrameRect.width()/2.0 - 5;
  
  QString label = metrics.elidedText(startValue().toString(), Qt::ElideRight,
    qRound(halfWidth));
  QPointF pt(keyFrameRect.left()+3.0, 
            keyFrameRect.top() + 0.5*keyFrameRect.height() + metrics.height() / 2.0 - 1.0);
  p->drawText(pt, label);
  
  label = metrics.elidedText(endValue().toString(), Qt::ElideRight,
    qRound(halfWidth));
  pt = QPointF(keyFrameRect.right() - metrics.width(label) - 3.0, 
            keyFrameRect.top() + 0.5*keyFrameRect.height() + metrics.height() / 2.0 - 1.0);
  
  p->drawText(pt, label);

  p->restore();
}

