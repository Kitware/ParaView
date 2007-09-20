
#include <QActionGroup>
#include <QString>
class pqServerManagerModelItem;
class pqPipelineSource;
class pqOutputPort;
class vtkObject;
class vtkEventQtSlotConnect;
class pqDataRepresentation;

class PrismToolBarActions : public QActionGroup
{
  Q_OBJECT
public:
  PrismToolBarActions(QObject* p);
  ~PrismToolBarActions();

public slots:
  void onSESAMEFileOpen();

private slots:
  void onSelectionChanged();
  void onGeometrySelection(vtkObject* caller, unsigned long vtk_event, void* client_data, void* call_data);
  void onPrismSelection(vtkObject* caller, unsigned long vtk_event, void* client_data, void* call_data);
  void onPrismRepresentationAdded(pqPipelineSource* source,pqDataRepresentation* repr, int srcOutputPort);
  void onConnectionAdded(pqPipelineSource* source,pqPipelineSource* consumer);


private:
  pqServerManagerModelItem *getActiveObject() const;

 QAction *SesameViewAction;
 pqPipelineSource* createFilterForActiveSource(
  const QString& xmlname);

 vtkEventQtSlotConnect* VTKConnections;
 bool ProcessingEvent;


};

