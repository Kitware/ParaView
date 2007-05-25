
#include "pqFlatTreeView.h"

#include <QTimer>
#include <QStandardItemModel>
#include <QItemSelectionModel>

#include "QTestApp.h"

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define VERIFY(a) if(!(a)) \
  { qWarning("case failed at line " TOSTRING(__LINE__) " :\n\t" TOSTRING(a)); }

void FlatTreeViewTests(pqFlatTreeView* w)
{
  QTestApp::keyClick(w, Qt::Key_Down, 0, 20);
  VERIFY(w->getSelectionModel()->currentIndex() == w->getModel()->index(0, 0));

  QTestApp::delay(20);
  w->collapse(w->getModel()->index(1,0));
  QTestApp::delay(20);
  w->expand(w->getModel()->index(1,0));
  QTestApp::delay(20);
  w->setCurrentIndex(w->getModel()->index(1,0));
  VERIFY(w->getSelectionModel()->currentIndex() == w->getModel()->index(1, 0));
  
  QTestApp::mouseDown(w->viewport(), QPoint(31, 38), Qt::LeftButton, 0, 20);
  VERIFY(w->getSelectionModel()->currentIndex() == w->getModel()->index(0, 0));

  QTestApp::mouseDown(w->viewport(), QPoint(31, 38), Qt::LeftButton, 0, 20);
  QTestApp::keyClick(QApplication::focusWidget(), Qt::Key_A, 0, 20);
  QTestApp::keyClick(QApplication::focusWidget(), Qt::Key_B, Qt::ShiftModifier, 20);
  QTestApp::keyClick(QApplication::focusWidget(), Qt::Key_C, 0, 20);
  QTestApp::keyDown(QApplication::focusWidget(), Qt::Key_Enter, 0, 20);
  QTestApp::mouseDown(w->viewport(), QPoint(31, 59), Qt::LeftButton, 0, 20);

}

int FlatTreeView(int argc, char* argv[])
{
  QTestApp app(argc, argv);

  pqFlatTreeView treeView;
  QStandardItemModel* model = new QStandardItemModel(0, 1, &treeView);
  treeView.setModel(model);

  model->setHorizontalHeaderLabels(QStringList("header"));
  model->appendRow(new QStandardItem("aaa"));
  QStandardItem* item = new QStandardItem("bbb");
  item->appendRow(new QStandardItem("bbba"));
  item->appendRow(new QStandardItem("bbbb"));
  model->appendRow(item);

  model->appendRow(new QStandardItem("ccc"));

  treeView.expandAll();
  treeView.show();

  FlatTreeViewTests(&treeView);
  
  if(QCoreApplication::arguments().contains("--exit"))
    {
    QTimer::singleShot(100, QApplication::instance(), SLOT(quit()));
    }
  
  int status = QTestApp::exec();

  return status;
}

