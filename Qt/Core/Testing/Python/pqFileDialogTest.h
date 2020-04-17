
#include <QPointer>
#include <QWidget>
class QPushButton;
class QLabel;
class QComboBox;
class QLineEdit;

class pqServer;
#include "pqTestUtility.h"

class pqFileDialogTestUtility : public pqTestUtility
{
  Q_OBJECT
public:
  pqFileDialogTestUtility();
  ~pqFileDialogTestUtility();
  virtual bool playTests(const QStringList& filenames);
public Q_SLOTS:
  void playTheTests(const QStringList&);

protected:
  void setupFiles();
  void cleanupFiles();
};

// our main window
class pqFileDialogTestWidget : public QWidget
{
  Q_OBJECT
public:
  pqFileDialogTestWidget();

  pqTestUtility* Tester() { return &this->TestUtility; }

public Q_SLOTS:
  void record();
  void openFileDialog();
  void emittedFiles(const QList<QStringList>& files);

protected:
  QComboBox* FileMode;
  QComboBox* ConnectionMode;
  QLineEdit* FileFilter;
  QPushButton* OpenButton;
  QLabel* EmitLabel;
  QLabel* ReturnLabel;
  QPointer<pqServer> Server;
  pqFileDialogTestUtility TestUtility;
};
