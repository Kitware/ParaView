#ifndef __pqAdaptiveControls_h
#define __pqAdaptiveControls_h

#include <QDockWidget>

class pqDataRepresentation;
class pqAdaptiveRenderView;
class pqPipelineSource;
class pqPropertyLinks;
class QCheckBox;
class QComboBox;
class QPushButton;
class QSpinBox;

class pqAdaptiveControls : public QDockWidget
{
  Q_OBJECT
public:
  pqAdaptiveControls(QWidget* p);
  ~pqAdaptiveControls();

public slots:

  void onRestart();
  void onInterrupt();

  void onRefinementMode(int type);
  void onCoarsen();
  void onRefine();
  void onSetMaxDepth(int maxD);

private slots:

  //changes widgets to reflect the currently active representation and view
  void updateTrackee();
  void connectToRepresentation(pqPipelineSource*, pqDataRepresentation*);

private:

  pqDataRepresentation *currentRep;
  pqAdaptiveRenderView *currentView;

  pqPropertyLinks *propertyLinks;
  QPushButton *restartButton;
  QCheckBox *lockButton;
  QCheckBox *boundsButton;
  QSpinBox *maxDepthSpinner;

  QPushButton *interruptButton;

  QComboBox *refinementModeButton;
  QPushButton *coarsenButton;
  QPushButton *refineButton;

  class pqInternals;
  pqInternals* Internals;
};

#endif // __pqAdaptiveControls_h
