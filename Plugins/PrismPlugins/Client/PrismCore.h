

#ifndef _PrismCore_h
#define _PrismCore_h
#include <QObject>
#include <QString>
#include "vtkSmartPointer.h"
#include <QItemDelegate>
#include "PrismSurfacePanel.h"
#include "PrismPanel.h"

class pqServerManagerModelItem;
class pqPipelineSource;
class pqServer;
class pqOutputPort;
class vtkObject;
class vtkEventQtSlotConnect;
class pqDataRepresentation;
class QAction;
class QActionGroup;
class pqRenderView;
class PrismScaleViewDialog;

class PrismCore : public QObject
{
  Q_OBJECT
public:
  PrismCore(QObject* p);
  ~PrismCore();

   static PrismCore* instance();

  void registerActions(QAction* prismView, QAction* sesameSurface,
    QAction* scaleView);

signals:
  void prismViewCreatable(bool);

public slots:
  void onSESAMEFileOpen();
  void onSESAMEFileOpen(const QStringList&);
  void onCreatePrismView();
  void onCreatePrismView(const QStringList& files);
  void onChangePrismViewScale();

private slots:
  void onSelectionChanged();
  void onGeometrySelection(vtkObject* caller, unsigned long vtk_event, void* client_data, void* call_data);
  void onPrismSelection(vtkObject* caller, unsigned long vtk_event, void* client_data, void* call_data);
  void onConnectionAdded(pqPipelineSource* source,pqPipelineSource* consumer);

private:  
  pqPipelineSource *getActiveSource() const;
  pqServer* getActiveServer() const;

 vtkSmartPointer<vtkEventQtSlotConnect> VTKConnections;
 PrismScaleViewDialog* ScaleViewDialog;
 bool ProcessingEvent;
};

class SESAMEComboBoxDelegate : public QItemDelegate
 {
     Q_OBJECT

 public:
     SESAMEComboBoxDelegate(QObject *parent = 0);

     QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                           const QModelIndex &index) const;

     void setEditorData(QWidget *editor, const QModelIndex &index) const;
     void setModelData(QWidget *editor, QAbstractItemModel *model,
                       const QModelIndex &index) const;

     void setVariableList(QStringList &variables);
     void updateEditorGeometry(QWidget *editor,
         const QStyleOptionViewItem &option, const QModelIndex &index) const;

     void setPanel(PrismSurfacePanel*);
     void setPanel(PrismPanel*);
protected:
     QStringList Variables;
     PrismSurfacePanel* SPanel;
     PrismPanel* PPanel;
 };


class  PrismTableWidget : public QTableWidget
{
  Q_OBJECT
public:

  PrismTableWidget(QWidget* p = NULL);
  ~PrismTableWidget();

  /// give a hint on the size
  QSize sizeHint() const;
  QSize minimumSizeHint() const;
protected slots:
  void invalidateLayout();

protected:
};


#endif


