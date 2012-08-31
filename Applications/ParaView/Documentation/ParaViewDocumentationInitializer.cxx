#include "ParaViewDocumentationInitializer.h"

#include "vtkPVConfig.h"
#include <QObject>
#include <QtPlugin>

void PARAVIEW_DOCUMENTATION_INIT()
{
  Q_INIT_RESOURCE(paraview_documentation);
}
