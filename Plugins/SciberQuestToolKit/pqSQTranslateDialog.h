#ifndef __pqSQTranslateDialog_h
#define __pqSQTranslateDialog_h

// .NAME pqSQTranslateDialog - Dialog for entering translation
// .SECTION Description
// .SECTION See Also
// .SECTION Thanks

#include <QDialog>

class pqSQTranslateDialogUI;

class pqSQTranslateDialog : public QDialog
{
Q_OBJECT

public:
  pqSQTranslateDialog(QWidget *parent,Qt::WindowFlags f);
  ~pqSQTranslateDialog();

  void GetTranslation(double *t);

  double GetTranslateX();
  double GetTranslateY();
  double GetTranslateZ();

  bool GetTypeIsNewOrigin();
  bool GetTypeIsOffset();

private:
  pqSQTranslateDialogUI *Ui;
};

#endif
