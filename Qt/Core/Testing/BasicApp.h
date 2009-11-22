
#include <QMainWindow>
#include <QPointer>
#include "pqRenderView.h"
#include "vtkObject.h"

class MainWindow : public QMainWindow
{
  Q_OBJECT
public:
  MainWindow();
  bool compareView(const QString& referenceImage, double threshold,
    ostream& output, const QString& tempDirectory);

  QPointer<pqRenderView> RenderView;

public slots:
  void processTest();

};
