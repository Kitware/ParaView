/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.

*/
#ifndef __pqSQVolumeSource_h
#define __pqSQVolumeSource_h

#include "pqNamedObjectPanel.h"
//#include "pqComponentsExport.h"
//#include <vtkstd/vector>

#include "ui_pqSQVolumeSourceForm.h"
using Ui::pqSQVolumeSourceForm;

// Define the following to enable debug io
// #define pqSQVolumeSourceDEBUG

class pqProxy;
class vtkEventQtSlotConnect;
class QWidget;

class pqSQVolumeSource : public pqNamedObjectPanel
{
  Q_OBJECT
public:
  pqSQVolumeSource(pqProxy* proxy, QWidget* p=NULL);
  ~pqSQVolumeSource();

  // Description:
  // Set/Get values to/from the UI.
  void GetOrigin(double *o);
  void SetOrigin(double *o);

  void GetPoint1(double *p1);
  void SetPoint1(double *p1);

  void GetPoint2(double *p2);
  void SetPoint2(double *p2);

  void GetPoint3(double *p3);
  void SetPoint3(double *p3);

  void GetResolution(int *res);
  void SetResolution(int *res);

  void GetSpacing(double *dx);
  void SetSpacing(double *dx);

  // Description:
  // dispatch context menu events.
  void contextMenuEvent(QContextMenuEvent *event);

protected slots:
  // Description:
  // Transfer configuration to and from the clip board.
  void CopyConfiguration();
  void PasteConfiguration();

  // Description:
  // read/write configuration from disk.
  void loadConfiguration();
  void saveConfiguration();

  // Description:
  // check if cooridnates produce a good volume.
  int ValidateCoordinates();

  // Description:
  // calculate plane's dimensions for display. Retun 0 in case
  // an error occured.
  void DimensionsModified();

  // Description:
  // update and display computed values, and enforce aspect ratio lock.
  void SpacingModified();
  void ResolutionModified();

  // Description:
  // Update the UI with values from the server.
  void PullServerConfig();
  void PushServerConfig();

  // Description:
  // This is where we have to communicate our state to the server.
  virtual void accept();

  // Description:
  // Pull config from proxy
  virtual void reset();

private:
  double Dims[3];
  double Dx[3];
  int Nx[3];

private:
  pqSQVolumeSourceForm *Form;
  vtkEventQtSlotConnect *VTKConnect;
};

#endif
