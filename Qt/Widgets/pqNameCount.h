
/// \file pqNameCount.h
/// \date 12/9/2005

#ifndef _pqNameCount_h
#define _pqNameCount_h


#include "QtWidgetsExport.h"

class pqNameCountInternal;
class QString;


class QTWIDGETS_EXPORT pqNameCount
{
public:
  pqNameCount();
  ~pqNameCount();

  unsigned int GetCount(const QString &name);
  unsigned int GetCountAndIncrement(const QString &name);
  void IncrementCount(const QString &name);
  void SetCount(const QString &name, unsigned int count);

  void Reset();

private:
  pqNameCountInternal *Internal;
};

#endif
