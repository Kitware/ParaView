#include "ParaViewDocumentationInitializer.h"

#include "vtkPVConfig.h"
#include <QObject>
#include <QtPlugin>

void PARAVIEW_DOCUMENTATION_INIT()
{
#ifndef BUILD_SHARED_LIBS
  Q_INIT_RESOURCE(paraview_documentation);
#endif
}
