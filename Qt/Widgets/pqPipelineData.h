#ifndef _pqPipelineData_h
#define _pqPipelineData_h

#include <QObject>

class pqNameCount;
class pqPipelineDataInternal;
class pqPipelineObject;
class pqPipelineServer;
class pqPipelineWindow;
class pqServer;
class QVTKWidget;
class vtkSMProxy;
class vtkSMRenderModuleProxy;
class vtkSMSourceProxy;
class vtkSMProxy;

#include "QtWidgetsExport.h"
#include <QObject>

/// interface for querying pipline state, also provides signals for pipeline changes
class QTWIDGETS_EXPORT pqPipelineData : public QObject
{
  Q_OBJECT

public:
  pqPipelineData(QObject* parent=0);
  virtual ~pqPipelineData();

  static pqPipelineData *instance();

  void addServer(pqServer *server);
  void removeServer(pqServer *server);

  void addWindow(QVTKWidget *window, pqServer *server);
  void removeWindow(QVTKWidget *window);

  void addViewMapping(QVTKWidget *widget, vtkSMRenderModuleProxy *module);
  vtkSMRenderModuleProxy *removeViewMapping(QVTKWidget *widget);
  vtkSMRenderModuleProxy *getRenderModule(QVTKWidget *widget) const;
  void clearViewMapping();

  vtkSMProxy *createSource(const char *proxyName, QVTKWidget *window);
  vtkSMProxy *createFilter(const char *proxyName, QVTKWidget *window);

  void setVisibility(vtkSMProxy *proxy, bool on);

  void addInput(vtkSMProxy *proxy, vtkSMProxy *input);
  void removeInput(vtkSMProxy *proxy, vtkSMProxy *input);

  QVTKWidget *getWindowFor(vtkSMProxy *proxy) const;
  pqPipelineObject *getObjectFor(vtkSMProxy *proxy) const;
  pqPipelineWindow *getObjectFor(QVTKWidget *window) const;

signals:
  void serverAdded(pqPipelineServer *server);
  void removingServer(pqPipelineServer *server);

  void windowAdded(pqPipelineWindow *window);
  void removingWindow(pqPipelineWindow *window);

  void sourceCreated(pqPipelineObject *source);
  void filterCreated(pqPipelineObject *filter);

  void connectionCreated(pqPipelineObject *source, pqPipelineObject *sink);
  void removingConnection(pqPipelineObject *source, pqPipelineObject *sink);

  /// signal for new proxy created
  void newPipelineObject(vtkSMSourceProxy* proxy);
  /// signal for new input connection to proxy
  void newPipelineObjectInput(vtkSMSourceProxy* proxy, vtkSMSourceProxy* input);

  /// new current proxy
  void currentProxyChanged(vtkSMSourceProxy* newproxy);

public slots:
  /// set the current proxy
  void setCurrentProxy(vtkSMSourceProxy* proxy);

public:
  /// get the current proxy
  vtkSMSourceProxy* currentProxy() const;

protected:
  pqPipelineDataInternal *Internal; ///< Stores the pipeline objects.
  pqNameCount *Names;               ///< Keeps track of the proxy names.
  vtkSMSourceProxy* CurrentProxy;

  static pqPipelineData *Instance;  ///< Pointer to the pipeline instance.
};

#endif // _pqPipelineData_h

