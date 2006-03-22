
/// \file pqPipelineServer.h
///
/// \date 11/16/2005

#ifndef _pqPipelineServer_h
#define _pqPipelineServer_h


#include "QtWidgetsExport.h"

class pqMultiView;
class pqPipelineObject;
class pqPipelineServerInternal;
class pqPipelineWindow;
class pqServer;
class QWidget;
class vtkPVXMLElement;
class vtkSMProxy;


class QTWIDGETS_EXPORT pqPipelineServer
{
public:
  pqPipelineServer();
  ~pqPipelineServer();

  void SaveState(vtkPVXMLElement *root, pqMultiView *multiView=0);

  void SetServer(pqServer *server) {this->Server = server;}
  pqServer *GetServer() const {return this->Server;}

  pqPipelineObject *AddSource(vtkSMProxy *source);
  pqPipelineObject *AddFilter(vtkSMProxy *filter);
  pqPipelineObject *AddCompoundProxy(vtkSMProxy *proxy);
  pqPipelineWindow *AddWindow(QWidget *window);

  pqPipelineObject *GetObject(vtkSMProxy *proxy) const;
  pqPipelineWindow *GetWindow(QWidget *window) const;

  bool RemoveObject(vtkSMProxy *proxy);
  bool RemoveWindow(QWidget *window);

  int GetSourceCount() const;
  pqPipelineObject *GetSource(int index) const;

  int GetWindowCount() const;
  pqPipelineWindow *GetWindow(int index) const;

  void ClearPipelines();

private:
  void UnregisterObject(pqPipelineObject *object);

private:
  pqPipelineServerInternal *Internal;
  pqServer *Server;
};

#endif
