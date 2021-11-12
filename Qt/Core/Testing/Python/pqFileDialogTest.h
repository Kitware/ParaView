
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
  ~pqFileDialogTestUtility() override;
  bool playTests(const QStringList& filenames) override;
public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  void playTheTests();

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

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
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
