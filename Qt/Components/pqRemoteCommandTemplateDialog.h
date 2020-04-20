#ifndef pqRemoteCommandTemplateDialog_h
#define pqRemoteCommandTemplateDialog_h
// .NAME pqRemoteCommandTemplateDialog - Dialog for configuring server side signals
// .SECTION Description

#include <QDialog>
#include <QLineEdit>
#include <QList>
#include <QString>

class pqRemoteCommandTemplateDialogUI;

class pqRemoteCommandTemplateDialog : public QDialog
{
  Q_OBJECT

public:
  pqRemoteCommandTemplateDialog(QWidget* parent, Qt::WindowFlags f);
  ~pqRemoteCommandTemplateDialog() override;

  // Description:
  // Access to UI state
  void SetCommandName(QString name);
  QString GetCommandName();

  void SetCommandTemplate(QString templ);
  QString GetCommandTemplate();

  int GetModified() { return this->Modified; }

private Q_SLOTS:
  void SetModified() { ++this->Modified; }

private:
  int Modified;
  pqRemoteCommandTemplateDialogUI* Ui;
};

#endif
