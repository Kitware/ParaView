/*=========================================================================

   Program: ParaView
   Module:    PrismSurfacePanel.h

=========================================================================*/
#ifndef _PrismSurfacePanel_h
#define _PrismSurfacePanel_h

#include <QWidget>
#include <QVariant>
#include "pqPipelineRepresentation.h"
#include "vtkSMProxy.h"
#include "pqNamedObjectPanel.h"




/// Widget which provides an editor for the properties of a display.
class  PrismSurfacePanel : public pqNamedObjectPanel
{
  Q_OBJECT
  
public:
  /// constructor
  PrismSurfacePanel(pqProxy* proxy, QWidget* p = NULL);
  /// destructor
  ~PrismSurfacePanel();


public slots:
  
  /// reset changes made by this panel
  virtual void accept();
  virtual void reset();

protected:
  /// populate widgets with properties from the server manager
  virtual void linkServerManagerProperties();

  // fill the parameters part of the GUI
  void setupVariables();
  void setupTableWidget();
  void updateVariables();
 // void setupLogScaling();
  void updateXThresholds();
  void setupXThresholds();

  void updateYThresholds();
  void setupYThresholds();

  class pqUI;
  pqUI* UI;

protected slots:
  void setTableId(QString);
  void setXVariable(QString);
  void setYVariable(QString);
  void setZVariable(QString);
  void setContourVariable(QString);
  void lowerXChanged(double);
  void upperXChanged(double);
  void lowerYChanged(double);
  void upperYChanged(double);

  void useXLogScaling(bool);
  void useYLogScaling(bool);

};

#endif
