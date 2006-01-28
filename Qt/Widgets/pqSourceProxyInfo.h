
/// \file pqSourceProxyInfo.h
///
/// \date 1/27/2006

#ifndef _pqSourceProxyInfo_h
#define _pqSourceProxyInfo_h


#include "QtWidgetsExport.h"

class pqSourceProxyInfoInternal;
class QString;
class QStringList;
class vtkPVXMLElement;


class QTWIDGETS_EXPORT pqSourceProxyInfo
{
public:
  pqSourceProxyInfo();
  ~pqSourceProxyInfo();

  void Reset();

  bool IsFilterInfoLoaded() const;
  void LoadFilterInfo(vtkPVXMLElement *root);

  void GetFilterMenu(QStringList &menuList) const;
  void GetFilterCategories(const QString &name, QStringList &list) const;
  void GetFilterMenuCategories(const QString &name, QStringList &list) const;

private:
  pqSourceProxyInfoInternal *Internal;
};

#endif
