
#include <QMainWindow>
#include <QPointer>
#include <QComboBox>
#include <QStringList>

#include "pqPipelineModel.h"
#include "pqPipelineBrowserWidget.h"
#include "vtkObject.h"

class pqServer;

class MainPipelineWindow : public QMainWindow
{
  Q_OBJECT
public:
  MainPipelineWindow();
  void createPipelineWithAnnotation(pqServer* server);

protected:
  QStringList                       FilterNames;
  QPointer<QComboBox>               FilterSelector;
  QPointer<pqPipelineBrowserWidget> PipelineWidget;

public slots:
  void processTest();
  void updateSelectedFilter(int);
  void showSettings();
};
