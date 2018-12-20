#include "ParaViewDocumentationInitializer.h"

#include <QObject>
#include <QtPlugin>

void paraview_documentation_initialize()
{
  Q_INIT_RESOURCE(paraview_documentation);
}
