/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.

*/
#ifndef pqSQHemisphereSource_h
#define pqSQHemisphereSource_h

#include "pqNamedObjectPanel.h"
#include "ui_pqSQHemisphereSourceForm.h" // for ui

using Ui::pqSQHemisphereSourceForm;

class pqProxy;
class pqPropertyLinks;
class vtkEventQtSlotConnect;
class QWidget;

class pqSQHemisphereSource : public pqNamedObjectPanel
{
  Q_OBJECT
public:
  pqSQHemisphereSource(pqProxy* proxy, QWidget* p = NULL);
  ~pqSQHemisphereSource();

protected slots:
  // Description:
  // read state from disk.
  void Restore();
  void loadConfiguration();
  // Description:
  // write state to disk.
  void Save();
  void saveConfiguration();

  // Description:
  // Update the UI with values from the server.
  void PullServerConfig();
  void PushServerConfig();

  // Description:
  // This is where we have to communicate our state to the server.
  void accept();

  //Description:
  // UI driven reset of widget to current server manager values.
  void reset();

private:
  pqSQHemisphereSourceForm *Form;
  pqPropertyLinks *Links;
};

#endif
