/*=========================================================================

   Program:   ParaView
   Module:    pqSignalAdaptorKeyFrameTime.h

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
#ifndef __pqSignalAdaptorKeyFrameTime_h
#define __pqSignalAdaptorKeyFrameTime_h

#include "pqComponentsExport.h"
#include <QObject>

class pqAnimationCue;
class pqAnimationScene;

/// pqSignalAdaptorKeyFrameTime is adaptor for key frame's time.
/// We use the adptor to scale the actual SMProperty value since
/// the SMProperty is usually normalized while the GUI should show
/// it in human readable form i.e. scaled with respect to the scene's
/// clock time range.
class PQCOMPONENTS_EXPORT pqSignalAdaptorKeyFrameTime : public QObject
{
  Q_OBJECT
  Q_PROPERTY(double normalizedTime READ normalizedTime WRITE setNormalizedTime)
public:
  /// Constructor. \c object is the QObject showing the time in the GUI
  /// \c propertyname is the Qt property to get/set the GUI's time value
  /// \c signal is the signal emitted when the GUI changes.
  pqSignalAdaptorKeyFrameTime(QObject* object, 
    const QString& propertyname, const QString& signal);
  virtual ~pqSignalAdaptorKeyFrameTime();

  /// Set the time keeper. Time keeper provides us
  /// with the range for the time values.
  void setAnimationScene(pqAnimationScene* keeper);

  /// Set the AnimationCue. It is needed to know the time mode of the key frame
  /// itself. The user may explicitly change the time to relative, in which
  /// case we don't want to crash and burn!
  void setAnimationCue(pqAnimationCue* cue);

  /// Get normalized Time. 
  double normalizedTime() const;

signals:
  /// fired when the GUI changes i.e. when the signal indicated in the
  /// constructor of this class is fired.
  void timeChanged();

public slots:
  void setNormalizedTime(double time);

protected slots:
  void timeRangesChanged();

private:
  pqSignalAdaptorKeyFrameTime(const pqSignalAdaptorKeyFrameTime&); // Not implemented.
  void operator=(const pqSignalAdaptorKeyFrameTime&); // Not implemented.

  class pqInternals;
  pqInternals* Internals;
};

#endif


