/*=========================================================================

   Program: ParaView
   Module:    pqNameCount.h

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2. 

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

/// \file pqNameCount.h
/// \brief
///   The pqNameCount class is used to associate a count to a name.
///
/// \date 12/9/2005

#ifndef _pqNameCount_h
#define _pqNameCount_h


#include "pqCoreExport.h"

class pqNameCountInternal;
class QString;


/// \class pqNameCount
/// \brief
///   The pqNameCount class is used to associate a count to a name.
///
/// The count associated with a name string can be incremented or
/// set to a specific value. The name/count map can be reset as well.
class PQCORE_EXPORT pqNameCount
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
