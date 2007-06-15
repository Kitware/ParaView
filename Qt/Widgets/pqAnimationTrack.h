
#ifndef pqAnimationTrack_h
#define pqAnimationTrack_h

#include <QObject>
#include <QGraphicsItem>
#include <QList>

class pqAnimationKeyFrame;

// represents a track
class pqAnimationTrack : public QObject, public QGraphicsItem
{
  Q_OBJECT
  Q_PROPERTY(QVariant property READ property WRITE setProperty)
  Q_PROPERTY(double startTime READ startTime WRITE setStartTime)
  Q_PROPERTY(double endTime READ endTime WRITE setEndTime)
public:

  pqAnimationTrack(QObject* p = 0);
  ~pqAnimationTrack();
  
  int count();
  pqAnimationKeyFrame* keyFrame(int);

  pqAnimationKeyFrame* addKeyFrame();
  void removeKeyFrame(pqAnimationKeyFrame* frame);

  QVariant property() const;
  
  QRectF boundingRect() const;
  
  double startTime() const;
  double endTime() const;

public slots:
  void setProperty(const QVariant& p);

  void setStartTime(double t);
  void setEndTime(double t);
  
  void setBoundingRect(const QRectF& r);

signals:
  void propertyChanged();

protected:

  void adjustKeyFrameRects();

  virtual void paint(QPainter* p,
                     const QStyleOptionGraphicsItem * option,
                     QWidget * widget);


private:

  double StartTime;
  double EndTime;

  QList<pqAnimationKeyFrame*> Frames;
  QVariant Property;

  QRectF Rect;

};

#endif // pqAnimationTrack_h

