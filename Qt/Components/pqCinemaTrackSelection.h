/*=========================================================================

   Program: ParaView
   Module:  pqCinemaTrackSelection.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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

========================================================================*/
#ifndef pqCinemaTrackSelection_h
#define pqCinemaTrackSelection_h

#include <map>

#include "pqComponentsModule.h"
#include <QWidget>

namespace Ui
{
class CinemaTrackSelection;
}

class QModelIndex;

class pqPipelineFilter;
class pqCinemaTrack;
class pqPipelineModel;
class pqPipelineAnnotationFilterModel;
class pqPipelineSource;
class pqArraySelectionModel;

/**
* @brief Widget to select among supported Cinema Tracks (filters).
*
* The user selection can be queried as a string to be included in a Python
* script directly. The widget is used by pqCinemaConfiguration and
* pqSGExportStateWizard.
*/
class PQCOMPONENTS_EXPORT pqCinemaTrackSelection : public QWidget
{
  Q_OBJECT;

public:
  typedef std::pair<pqArraySelectionModel*, pqCinemaTrack*> ItemValues;
  typedef std::map<pqPipelineSource*, ItemValues> ItemValuesMap;

  pqCinemaTrackSelection(QWidget* parent_ = NULL);
  ~pqCinemaTrackSelection();

  QList<pqCinemaTrack*> getTracks();

  /**
  * Returns a string containing a comma separated set of cinema tracks each with
  * a set of user selected values. Each track is defined as in 'format'.
  * Order of track values:
  * 1. Track Name
  * 2. Value tuple
  *
  * Example: Format as defined in pqCinemaConfiguration
  * format = "'%1' : 2%"
  * returns -> 'name1' : [a, b, c], 'name2' : [d, e, f], ... (for N tracks)
  */
  QString getTrackSelectionAsString(QString const& format);

  /**
  * Returns a string containing a comma separated set of cinema tracks each with
  * a set of user selected arrays. Each track is defined as in 'format'.
  * Order of track array names:
  * 1. Track Name
  * 2. Array names tuple
  * Example: Format as defined in pqCinemaConfiguration
  * format = "'%1' : %2"
  * returns -> 'nameTrack1' : ['array1', 'array2', ...], 'nameTrack1' : ['array1', ...], ...
  */
  QString getArraySelectionAsString(QString const& format);

  /**
  * Creates a PipelineModel which gets populated using the current
  * ServerManagerModel and passes it to the View object.
  */
  void initializePipelineBrowser();

public slots:
  void enableArraySelection(bool enable);

private slots:

  /**
  * Selection change handler.
  */
  void onPipelineItemChanged(QModelIndex const& current, QModelIndex const& previous);

private:
  /**
  * Creates cinema track widgets (for value customization) and value
  * array selection models.
  * @note Only some filters are currently supported by Cinema.
  */
  void initializePipelineItemValues(QList<pqPipelineSource*> const& items);

  pqPipelineSource* getPipelineSource(QModelIndex const& index) const;

  /**
  *
  */

  Ui::CinemaTrackSelection* Ui;

  ItemValuesMap PipelineItemValues;
};

#endif
