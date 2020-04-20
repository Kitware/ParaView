
#include <QCheckBox>
#include <QComboBox>
#include <QMainWindow>
#include <QPointer>
#include <QStringList>

#include "pqPipelineBrowserWidget.h"
#include "pqPipelineModel.h"
#include "vtkObject.h"

class pqServer;

class MainPipelineWindow : public QMainWindow
{
  Q_OBJECT
public:
  MainPipelineWindow();
  void createPipelineWithAnnotation(pqServer* server);

protected:
  QStringList FilterNames;
  QPointer<QComboBox> FilterSelector;
  QPointer<pqPipelineBrowserWidget> PipelineWidget;
  QPointer<QCheckBox> InvertMatching;

public Q_SLOTS:
  void processTest();
  void updateSelectedFilter(int);
  void invertFilterMatching(int);
  void showSettings();
};
