
/// \file pqNameCount.h
/// \date 12/9/2005

#ifndef _pqNameCount_h
#define _pqNameCount_h


class pqNameCountInternal;
class QString;


class pqNameCount
{
public:
  pqNameCount();
  ~pqNameCount();

  unsigned int GetCount(const QString &name);
  unsigned int GetCountAndIncrement(const QString &name);
  void IncrementCount(const QString &name);

private:
  pqNameCountInternal *Internal;
};

#endif
