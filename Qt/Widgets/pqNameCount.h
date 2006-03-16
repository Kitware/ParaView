
/// \file pqNameCount.h
/// \brief
///   The pqNameCount class is used to associate a count to a name.
///
/// \date 12/9/2005

#ifndef _pqNameCount_h
#define _pqNameCount_h


#include "QtWidgetsExport.h"

class pqNameCountInternal;
class QString;


/// \class pqNameCount
/// \brief
///   The pqNameCount class is used to associate a count to a name.
///
/// The count associated with a name string can be incremented or
/// set to a specific value. The name/count map can be reset as well.
class QTWIDGETS_EXPORT pqNameCount
{
public:
  pqNameCount();
  ~pqNameCount();

  /// \brief
  ///   Gets the current count for the specified name.
  ///
  /// If the name does not exist in the map, a new entry is created
  /// and 1 is returned.
  ///
  /// \param name The name to look up.
  /// \return
  ///   The count associated with the specified name.
  unsigned int GetCount(const QString &name);

  /// \brief
  ///   Gets the current count for a name and then increments
  ///   the stored value.
  /// \param name The name to look up.
  /// \return
  ///   The count associated with the specified name.
  /// \sa pqNameCount::GetCount(const QString &)
  unsigned int GetCountAndIncrement(const QString &name);

  /// \brief
  ///   Increments the count for a specified name.
  /// \param name The name to look up.
  /// \sa pqNameCount::GetCountAndIncrement(const QString &)
  void IncrementCount(const QString &name);

  /// \brief
  ///   Sets the count for the specified name.
  /// \param name The name to look up.
  /// \param count The new count to assign to the name.
  void SetCount(const QString &name, unsigned int count);

  /// Resets the name/count map to an empty state.
  void Reset();

private:
  pqNameCountInternal *Internal; ///< Stores the name/count map.
};

#endif
