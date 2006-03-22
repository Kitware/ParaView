
/// \file pqPipelineWindow.h
///
/// \date 12/22/2005

#ifndef _pqPipelineWindow_h
#define _pqPipelineWindow_h


#include "QtWidgetsExport.h"

class pqPipelineServer;
class QWidget;


class QTWIDGETS_EXPORT pqPipelineWindow
{
public:
  pqPipelineWindow(QWidget *window);
  ~pqPipelineWindow() {}

  QWidget *GetWidget() const {return this->Widget;}
  void SetWidget(QWidget *widget) {this->Widget = widget;}

  pqPipelineServer *GetServer() const {return this->Server;}
  void SetServer(pqPipelineServer *server) {this->Server = server;}

private:
  QWidget *Widget;          ///< Stores the widget pointer.
  pqPipelineServer *Server; ///< Stores the parent server.
};

#endif
