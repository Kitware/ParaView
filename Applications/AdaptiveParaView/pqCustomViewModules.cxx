
#include "pqCustomViewModules.h"

pqCustomViewModules::pqCustomViewModules(QObject* o)
  : pqStandardViewModules(o)
{
}

pqCustomViewModules::~pqCustomViewModules()
{
}

QStringList pqCustomViewModules::viewTypes() const
{
  return QStringList() << "AdaptiveRenderView" ; 
}
