#ifndef __pqSQRemoteSignalDialog_h
#define __pqSQRemoteSignalDialog_h
// .NAME pqSQRemoteSignalDialog - Dialog for configuring server side signals
//
// .SECTION Description
//
// .SECTION See Also
//
// .SECTION Thanks

#include <QDialog>
#include <QLineEdit>
#include <QList>
#include <QStringList>
#include <QString>

class pqSQRemoteSignalDialogUI;

class pqSQRemoteSignalDialog : public QDialog
{
Q_OBJECT

public:
  pqSQRemoteSignalDialog(QWidget *parent, Qt::WindowFlags f);
  ~pqSQRemoteSignalDialog();

  // Description:
  // Access to UI state
  void SetTrapFPEDivByZero(int enable);
  int GetTrapFPEDivByZero();

  void SetTrapFPEInexact(int enable);
  int GetTrapFPEInexact();

  void SetTrapFPEInvalid(int enable);
  int GetTrapFPEInvalid();

  void SetTrapFPEOverflow(int enable);
  int GetTrapFPEOverflow();

  void SetTrapFPEUnderflow(int enable);
  int GetTrapFPEUnderflow();

  int GetModified(){ return this->Modified; }

private slots:
  void SetModified(){ ++this->Modified; }

private:
  int Modified;
  pqSQRemoteSignalDialogUI *Ui;
};

#endif
