
/// \file pqXMLUtil.h
///
/// \date 1/19/2006

#ifndef _pqXMLUtil_h
#define _pqXMLUtil_h


#include "QtWidgetsExport.h"
#include <QList>
#include <QString>

class vtkPVXMLElement;


namespace ParaQ
{
  QTWIDGETS_EXPORT vtkPVXMLElement *FindNestedElementByName(
      vtkPVXMLElement *element, const char *name);

  QTWIDGETS_EXPORT QString GetStringFromIntList(const QList<int> &list);
  QTWIDGETS_EXPORT QList<int> GetIntListFromString(const char *value);
};

#endif
