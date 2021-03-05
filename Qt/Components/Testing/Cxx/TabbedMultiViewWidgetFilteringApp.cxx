#include <QApplication>
#include <QComboBox>
#include <QMainWindow>
#include <QToolBar>
#include <QtDebug>

#include <pqActiveObjects.h>
#include <pqApplicationCore.h>
#include <pqObjectBuilder.h>
#include <pqServer.h>
#include <pqTabbedMultiViewWidget.h>
#include <vtkSMViewLayoutProxy.h>

namespace
{

class MainWindow : public QMainWindow
{
  QComboBox* ComboBox;
  pqTabbedMultiViewWidget* TMVWidget;

public:
  MainWindow()
  {
    auto tb = this->addToolBar("Toolbar");
    this->ComboBox = new QComboBox();
    this->ComboBox->addItem("No filtering", QString());
    tb->addWidget(this->ComboBox);

    this->TMVWidget = new pqTabbedMultiViewWidget(this);
    this->setCentralWidget(this->TMVWidget);

    this->setupPipeline();

    QObject::connect(
      this->ComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int) {
        const QString filter = this->ComboBox->currentData().toString();
        if (filter.isEmpty())
        {
          this->TMVWidget->disableAnnotationFilter();
        }
        else
        {
          this->TMVWidget->enableAnnotationFilter(filter);
        }
      });
  }

  ~MainWindow() override = default;

  void setupPipeline()
  {
    pqApplicationCore* core = pqApplicationCore::instance();
    pqObjectBuilder* ob = core->getObjectBuilder();

    // Create server only after a pipeline browser get created...
    pqServer* server = ob->createServer(pqServerResource("builtin:"));
    pqActiveObjects::instance().setActiveServer(server);

    QList<vtkSMViewLayoutProxy*> layouts;
    for (int cc = 0; cc < 4; ++cc)
    {
      this->TMVWidget->setCurrentTab(this->TMVWidget->createTab(server));
      layouts.push_back(this->TMVWidget->layoutProxy());
    }

    this->ComboBox->addItem("Filter 1 (Layouts 1,2,4)", QString("Filter1"));
    layouts[0]->SetAnnotation("Filter1", "1");
    layouts[1]->SetAnnotation("Filter1", "1");
    layouts[3]->SetAnnotation("Filter1", "1");

    this->ComboBox->addItem("Filter 2 (Layouts 3,4)", QString("Filter2"));
    layouts[2]->SetAnnotation("Filter2", "1");
    layouts[3]->SetAnnotation("Filter2", "1");
  }

  bool doTest()
  {
    this->ComboBox->setCurrentIndex(1);
    if (!this->validate({ "Layout #1", "Layout #2", "Layout #4", "+" }))
    {
      return false;
    }
    this->ComboBox->setCurrentIndex(2);
    if (!this->validate({ "Layout #3", "Layout #4", "+" }))
    {
      return false;
    }
    this->ComboBox->setCurrentIndex(0);
    if (!this->validate({ "Layout #1", "Layout #2", "Layout #3", "Layout #4", "+" }))
    {
      return false;
    }
    return true;
  }

  bool validate(const QStringList& labels) const
  {
    if (this->TMVWidget->visibleTabLabels() != labels)
    {
      qCritical() << "ERROR! Mismatched tabs. Expected: " << labels
                  << " Got: " << this->TMVWidget->visibleTabLabels();
      return false;
    }
    return true;
  }

private:
  Q_DISABLE_COPY(MainWindow);
};

} // end of namespace

int TabbedMultiViewWidgetFilteringApp(int argc, char* argv[])
{
  QApplication app(argc, argv);
  pqApplicationCore appCore(argc, argv);

  MainWindow window;
  window.resize(800, 600);
  window.show();

  const bool success = window.doTest();
  const int retval = app.arguments().indexOf("--exit") == -1 ? app.exec() : EXIT_SUCCESS;
  return success ? retval : EXIT_FAILURE;
}
