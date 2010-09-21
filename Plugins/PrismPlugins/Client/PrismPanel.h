/*=========================================================================

   Program: ParaView
   Module:    PrismPanel.h

=========================================================================*/
#ifndef _PrismPanel_h
#define _PrismPanel_h

#include <QWidget>
#include <QVariant>
#include "pqPipelineRepresentation.h"
#include "vtkSMProxy.h"
#include "pqNamedObjectPanel.h"

class QItemSelection;



/// Widget which provides an editor for the properties of a display.
class  PrismPanel : public pqNamedObjectPanel
{
  Q_OBJECT
  
public:
  /// constructor
  PrismPanel(pqProxy* proxy, QWidget* p = NULL);
  /// destructor
  ~PrismPanel();


public slots:
  
  /// reset changes made by this panel
  virtual void accept();
  virtual void reset();
  void onConversionVariableChanged(int);
protected:
  /// populate widgets with properties from the server manager
  virtual void linkServerManagerProperties();

  // fill the parameters part of the GUI
  void setupVariables();
  void setupTableWidget();
  void updateVariables();
  void setupConversions();
  void updateConversionsLabels();
  void updateConversions();
 // void setupLogScaling();
  void setupXThresholds();

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
 
  void updateXThresholds();
  void updateYThresholds();


  void useXLogScaling(bool);
  void useYLogScaling(bool);
  void useZLogScaling(bool);

  void onSamplesChanged();

  void onSelectionChanged(const QItemSelection&, const QItemSelection&);
  void onRangeChanged();

  void onDelete();
  void onDeleteAll();
  void onNewValue();
  void onNewRange();
  void onSelectAll();
  void onScientificNotation(bool);

  void onConversionFileButton();
  void onConversionTypeChanged(int);
  //void onDensityConversionChanged(const QString & text);
  //void onTemperatureConversionChanged(const QString & text);
  //void onPressureConversionChanged(const QString & text);
  //void onEnergyConversionChanged(const QString & text);
  void onConversionTreeCellChanged( int , int  );
 void updateVariableConversions();


private:
      bool eventFilter(QObject *object, QEvent *e);
  bool getRange(double& range_min, double& range_max);
  void updateConverstions();



};

#endif
