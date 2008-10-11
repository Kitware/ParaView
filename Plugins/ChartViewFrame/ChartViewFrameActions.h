#ifndef _ChartViewFrameActions_h
#define _ChartViewFrameActions_h

#include "pqViewFrameActionGroup.h"

class pqMultiViewFrame;
class pqView;

class ChartViewFrameActions : public pqViewFrameActionGroup
{
  Q_OBJECT
public:
  ChartViewFrameActions(QObject* p);
  ~ChartViewFrameActions();

  virtual bool connect(pqMultiViewFrame *frame, pqView *view);
  virtual bool disconnect(pqMultiViewFrame *frame, pqView *view);

private slots:

  // Description:
  // Updates the enable state of the toolbar actions when the 
  // pipline selection changes.
  void updateEnableState();

  // Description:
  void onZoomTypeChanged();
  void onResetAxes();
};
#endif

