#ifndef _ClientGraphViewFrameActions_h
#define _ClientGraphViewFrameActions_h

#include "pqViewFrameActionGroup.h"

class pqMultiViewFrame;
class pqNameCount;
class pqView;

class ClientGraphViewFrameActions : public pqViewFrameActionGroup
{
  Q_OBJECT
public:
  ClientGraphViewFrameActions(QObject* p);
  ~ClientGraphViewFrameActions();

  virtual bool connect(pqMultiViewFrame *frame, pqView *view);
  virtual bool disconnect(pqMultiViewFrame *frame, pqView *view);

private slots:
  void onResetCamera();
  void onZoomToSelection();
  void onExtractSubgraph();
  void onExpandSelection();

private:
  pqNameCount* NameGenerator;
};
#endif

