
#ifndef _pqOpenExodusOptions_h
#define _pqOpenExodusOptions_h

#include "QtWidgetsExport.h"
#include <QDialog>
#include "ui_pqOpenExodusOptions.h"

class vtkSMSourceProxy;

class QTWIDGETS_EXPORT pqOpenExodusOptions : public QDialog,
    public Ui::pqOpenExodusOptions
{
  Q_OBJECT
public:
  pqOpenExodusOptions(vtkSMSourceProxy* exodusReader, QWidget* p);
  ~pqOpenExodusOptions();

  virtual void accept();

private:
  vtkSMSourceProxy* ExodusReader;
};

#endif

