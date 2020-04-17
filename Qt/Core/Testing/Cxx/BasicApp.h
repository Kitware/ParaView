
#include "pqRenderView.h"
#include "vtkObject.h"
#include <QMainWindow>
#include <QPointer>

class MainWindow : public QMainWindow
{
  Q_OBJECT
public:
  MainWindow();
  bool compareView(
    const QString& referenceImage, double threshold, ostream& output, const QString& tempDirectory);
  QPointer<pqRenderView> RenderView;
public Q_SLOTS:
  void processTest();
};
