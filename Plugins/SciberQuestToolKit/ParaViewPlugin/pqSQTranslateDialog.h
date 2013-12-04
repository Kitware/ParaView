/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.

*/
#ifndef __pqSQTranslateDialog_h
#define __pqSQTranslateDialog_h

// .NAME pqSQTranslateDialog - Dialog for entering translation

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
