/*=========================================================================

   Program: ParaView
   Module:    pqChartSeriesColorManager.h

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

/// \file pqChartSeriesColorManager.h
/// \date 6/8/2007

#ifndef _pqChartSeriesColorManager_h
#define _pqChartSeriesColorManager_h


#include "QtChartExport.h"

class pqChartSeriesColorManagerInternal;
class pqChartSeriesOptionsGenerator;
class QObject;


/// \class pqChartSeriesColorManager
/// \brief
///   The pqChartSeriesColorManager class allows several chart layers
///   to share the same options generator.
///
/// Sharing an options generator keeps the series options from
/// repeating. This is useful when several of the same type of chart
/// layers are displayed in the same chart. For example, two line
/// charts can share an options generator to make sure that none of
/// the lines are the same color.
class QTCHART_EXPORT pqChartSeriesColorManager
{
public:
  pqChartSeriesColorManager();
  virtual ~pqChartSeriesColorManager();

  /// \brief
  ///   Gets the options generator.
  /// \return
  ///   A pointer to the options generator.
  pqChartSeriesOptionsGenerator *getGenerator();

  /// \brief
  ///   Sets the options generator.
  /// \param generator The new options generator.
  void setGenerator(pqChartSeriesOptionsGenerator *generator);

  /// \brief
  ///   Adds a series options object to the list.
  ///
  /// The index returned is the lowest index available. If there are
  /// empty spots from removals, the index will come from the first
  /// empty spot.
  ///
  /// \param options The options object to add.
  /// \return
  ///   The index for the series options generator.
  virtual int addSeriesOptions(const QObject *options);

  /// \brief
  ///   Removes a series options object from the list.
  ///
  /// When an object is removed, the empty spot is saved so it can be
  /// used by the next object added.
  ///
  /// \param options The options object to remove.
  virtual void removeSeriesOptions(const QObject *options);

private:
  /// Stores the series order.
  pqChartSeriesColorManagerInternal *Internal;
};

#endif
