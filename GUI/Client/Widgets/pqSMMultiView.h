
/// \file pqSMMultiView.h
///
/// \date 1/19/2006

#ifndef _pqSMMultiView_h
#define _pqSMMultiView_h

#include "QtWidgetsExport.h"

class pqMultiViewFrame;
class pqServer;
class QVTKWidget;
class QWidget;


namespace ParaQ
{
  QTWIDGETS_EXPORT QVTKWidget *AddQVTKWidget(pqMultiViewFrame *frame,
      QWidget *topWidget, pqServer *server);
};

#endif
