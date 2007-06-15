
#ifndef pqAnimationModel_h
#define pqAnimationModel_h

#include "QtWidgetsExport.h"

#include <QObject>
#include <QGraphicsScene>
#include <QStandardItemModel>

class pqAnimationTrack;
class QGraphicsView;

// represents a track
class QTWIDGETS_EXPORT pqAnimationModel : public QGraphicsScene
{
  Q_OBJECT
  Q_ENUMS(ModeType)
  Q_PROPERTY(ModeType mode READ mode WRITE setMode)
  Q_PROPERTY(double currentTime READ currentTime WRITE setCurrentTime)
  Q_PROPERTY(double startTime READ startTime WRITE setStartTime)
  Q_PROPERTY(double endTime READ endTime WRITE setEndTime)
public:

  enum ModeType
    {
    Real,
    Sequence,
    TimeSteps
    };

  pqAnimationModel(QGraphicsView* p = 0);
  ~pqAnimationModel();
  
  int count();
  pqAnimationTrack* track(int);

  pqAnimationTrack* addTrack();
  void removeTrack(pqAnimationTrack* track);

  ModeType mode() const;
  double currentTime() const;
  double startTime() const;
  double endTime() const;

  QAbstractItemModel* header();
  int rowHeight() const;

public slots:
  void setMode(ModeType);
  void setCurrentTime(double);
  void setStartTime(double);
  void setEndTime(double);

protected slots:

  void resizeTracks();
  void trackNameChanged();

protected:
  void drawForeground(QPainter* painter, const QRectF& rect);

  bool eventFilter(QObject* w, QEvent* e);

private:

  ModeType Mode;
  double CurrentTime;
  double StartTime;
  double EndTime;

  QList<pqAnimationTrack*> Tracks;

  // model that provides names of tracks
  QStandardItemModel Header;
};

#endif // pqAnimationModel_h

