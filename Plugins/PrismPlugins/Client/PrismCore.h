

#ifndef _PrismCore_h
#define _PrismCore_h
#include <QObject>
#include <QString>
class pqServerManagerModelItem;
class pqPipelineSource;
class pqServer;
class pqOutputPort;
class vtkObject;
class vtkEventQtSlotConnect;
class pqDataRepresentation;
class QAction;

class PrismCore : public QObject
{
  Q_OBJECT
public:
  PrismCore(QObject* p);
  ~PrismCore();

   static PrismCore* instance();

  void  actions(QList<QAction*>&);

public slots:
  void onSESAMEFileOpen();
  void onSESAMEFileOpen(const QStringList&);
  void onCreatePrismView();
  void onCreatePrismView(const QStringList& files);

private slots:
  void onSelectionChanged();
  void onGeometrySelection(vtkObject* caller, unsigned long vtk_event, void* client_data, void* call_data);
  void onPrismSelection(vtkObject* caller, unsigned long vtk_event, void* client_data, void* call_data);
  void onPrismRepresentationAdded(pqPipelineSource* source,pqDataRepresentation* repr, int srcOutputPort);
  void onConnectionAdded(pqPipelineSource* source,pqPipelineSource* consumer);


private:
  pqServerManagerModelItem *getActiveObject() const;
  pqPipelineSource *getActiveSource() const;
  pqServer* getActiveServer() const;

 QAction *SesameViewAction;
 QAction *PrismViewAction;

 vtkEventQtSlotConnect* VTKConnections;
 bool ProcessingEvent;


};
#endif


