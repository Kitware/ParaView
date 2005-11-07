
#ifndef _pqPipelineData
#define _pqPipelineData

class vtkSMSourceProxy;
class vtkSMProxy;
class pqServer;

#include <QObject>

/// interface for querying pipline state, also provides signals for pipeline changes
class pqPipelineData : public QObject
{
  Q_OBJECT

public:
  pqPipelineData(pqServer* server);
  ~pqPipelineData();

signals:
  /// signal for new proxy created
  void newPipelineObject(vtkSMSourceProxy* proxy);
  /// signal for new input connection to proxy
  void newPipelineObjectInput(vtkSMSourceProxy* proxy, vtkSMSourceProxy* input);

  /// new current proxy
  void currentProxyChanged(vtkSMSourceProxy* newproxy);

public slots:
  /// set the current proxy
  void setCurrentProxy(vtkSMSourceProxy* proxy);
  /// get the current proxy
  vtkSMSourceProxy* currentProxy() const;

public:
  /// create pipline object  \todo perhaps move this when server manager can emit event
  vtkSMProxy* newSMProxy(const char* groupname, const char* proxyname);
  /// create a pipline connection \todo perhaps move this when server manager can emit event
  void addInput(vtkSMSourceProxy* proxy, vtkSMSourceProxy* input);

protected:
  vtkSMSourceProxy* CurrentProxy;
  pqServer* Server;
};


#endif // _pqPipelineData


