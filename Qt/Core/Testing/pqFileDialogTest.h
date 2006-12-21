
#include <QWidget>
#include <QPointer>
class QPushButton;
class QLabel;
class QComboBox;
class QLineEdit;

class pqServer;
#include "pqTestUtility.h"


class pqFileDialogTestUtility : public pqTestUtility
{
public:
  pqFileDialogTestUtility();
  ~pqFileDialogTestUtility();
  void playTests(const QString& filename);
  void playTests(const QStringList& filenames);
  void testSucceeded();
  void testFailed();
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

public slots:
  void record();
  void openFileDialog();
  void emittedFiles(const QStringList& files);

protected:
  QComboBox*   FileMode;
  QComboBox*   ConnectionMode;
  QLineEdit*   FileFilter;
  QPushButton* OpenButton;
  QLabel*      EmitLabel;
  QLabel*      ReturnLabel;
  QPointer<pqServer> Server;
  pqFileDialogTestUtility TestUtility;
};
