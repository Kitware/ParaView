
#include "pqAnimationTrack.h"

#include <QPainter>

#include "pqAnimationKeyFrame.h"


pqAnimationTrack::pqAnimationTrack(QObject* p)
  : QObject(p), StartTime(0), EndTime(1), Rect(0,0,1,1)
{
}

pqAnimationTrack::~pqAnimationTrack()
{
}

int pqAnimationTrack::count()
{
  return this->Frames.size();
}

pqAnimationKeyFrame* pqAnimationTrack::keyFrame(int i)
{
  return this->Frames[i];
}

QRectF pqAnimationTrack::boundingRect() const
{ 
  return this->Rect;
}

void pqAnimationTrack::setBoundingRect(const QRectF& r)
{ 
  this->removeFromIndex();
  this->Rect = r;
  this->addToIndex();
  this->adjustKeyFrameRects();
}

void pqAnimationTrack::adjustKeyFrameRects()
{
  foreach(pqAnimationKeyFrame* f, this->Frames)
    {
    f->adjustRect();
    }
}

pqAnimationKeyFrame* pqAnimationTrack::addKeyFrame()
{
  pqAnimationKeyFrame* frame = new pqAnimationKeyFrame(this, this->scene());
  this->Frames.append(frame);
  return frame;
}

void pqAnimationTrack::removeKeyFrame(pqAnimationKeyFrame* frame)
{
  this->Frames.removeAll(frame);
  delete frame;
}


QVariant pqAnimationTrack::property() const
{
  return this->Property;
}


void pqAnimationTrack::setProperty(const QVariant& p)
{
  this->Property = p;
  emit this->propertyChanged();
}

void pqAnimationTrack::paint(QPainter* p,
                     const QStyleOptionGraphicsItem*,
                     QWidget *)
{
  // draw border for this track
  p->save();
  p->setBrush(QBrush(QColor(220,220,220)));
  QPen pen(QColor(0,0,0));
  pen.setWidth(0);
  p->setPen(pen);
  p->drawRect(this->boundingRect());
  p->restore();
}

double pqAnimationTrack::startTime() const
{
  return this->StartTime;
}
double pqAnimationTrack::endTime() const
{
  return this->EndTime;
}

void pqAnimationTrack::setStartTime(double t)
{
  this->StartTime = t;
  this->adjustKeyFrameRects();
}
void pqAnimationTrack::setEndTime(double t)
{
  this->EndTime = t;
  this->adjustKeyFrameRects();
}

