
#ifndef pqAnimationKeyFrame_h
#define pqAnimationKeyFrame_h

#include "QtWidgetsExport.h"

#include <QObject>
#include <QGraphicsItem>
class pqAnimationTrack;

// represents a key frame
class QTWIDGETS_EXPORT pqAnimationKeyFrame : public QObject, public QGraphicsItem
{
  Q_OBJECT
  Q_ENUMS(InterpolationType)
  Q_PROPERTY(double startTime READ startTime WRITE setStartTime)
  Q_PROPERTY(double endTime READ endTime WRITE setEndTime)
  Q_PROPERTY(QVariant startValue READ startValue WRITE setStartValue)
  Q_PROPERTY(QVariant endValue READ endValue WRITE setEndValue)
  Q_PROPERTY(InterpolationType interpolation READ interpolation
                                             WRITE setInterpolation)
public:

  enum InterpolationType
  {
    Linear
  };

  pqAnimationKeyFrame(pqAnimationTrack* p, QGraphicsScene* s);
  ~pqAnimationKeyFrame();

  double startTime() const;
  double endTime() const;
  QVariant startValue() const;
  QVariant endValue() const;
  InterpolationType interpolation() const;
  
  QRectF boundingRect() const;

public slots:
  void setStartTime(double t);
  void setEndTime(double t);
  void setStartValue(const QVariant&);
  void setEndValue(const QVariant&);
  void setInterpolation(InterpolationType);
  void setBoundingRect(const QRectF& r);
  void adjustRect();

signals:
  void startValueChanged();
  void endValueChanged();
  void interpolationChanged();

protected:

  virtual void paint(QPainter* p,
                     const QStyleOptionGraphicsItem * option,
                     QWidget * widget);
  

private:
  double StartTime;
  double EndTime;
  QVariant StartValue;
  QVariant EndValue;
  InterpolationType Interpolation;

  QRectF Rect;

};

#endif // pqAnimationKeyFrame_h

