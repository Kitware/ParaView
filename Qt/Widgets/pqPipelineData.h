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
class vtkSMCompoundProxy;
class vtkSMDisplayProxy;

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

  // TODO: perhaps not tie source proxies to windows

  vtkSMSourceProxy *createSource(const char *proxyName, QVTKWidget *window);
  vtkSMSourceProxy *createFilter(const char *proxyName, QVTKWidget *window);
  vtkSMCompoundProxy *createCompoundProxy(const char *proxyName, QVTKWidget *window);

  // TODO: should take a window to create a display proxy in, but source proxy is already tied to window
  //! create a display proxy for a source proxy
  vtkSMDisplayProxy* createDisplay(vtkSMSourceProxy* source, vtkSMProxy* rep = NULL);
  //! set the visibility on/off for a display
  void setVisibility(vtkSMDisplayProxy* proxy, bool on);

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
  void newPipelineObject(vtkSMProxy* proxy);
  /// signal for new input connection to proxy
  void newPipelineObjectInput(vtkSMProxy* proxy, vtkSMProxy* input);

  /// new current proxy
  void currentProxyChanged(vtkSMProxy* newproxy);

public slots:
  /// set the current proxy
  void setCurrentProxy(vtkSMProxy* proxy);

public:
  /// get the current proxy
  vtkSMProxy* currentProxy() const;

protected:
  pqPipelineDataInternal *Internal; ///< Stores the pipeline objects.
  pqNameCount *Names;               ///< Keeps track of the proxy names.
  vtkSMProxy* CurrentProxy;

  static pqPipelineData *Instance;  ///< Pointer to the pipeline instance.
};

#endif // _pqPipelineData_h

