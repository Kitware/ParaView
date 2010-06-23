/*=========================================================================

   Program: ParaView
   Module:    pqAnimationCue.h

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

========================================================================*/
#ifndef __pqAnimationCue_h
#define __pqAnimationCue_h

#include "pqProxy.h"

class vtkSMProxy;
class vtkSMProperty;

class PQCORE_EXPORT pqAnimationCue : public pqProxy
{
  Q_OBJECT
  typedef pqProxy Superclass;
public:
  pqAnimationCue(const QString& group, const QString& name,
    vtkSMProxy* proxy, pqServer* server, QObject* parent=NULL);
  virtual ~pqAnimationCue();

  // Returns the number of keyframes in this cue.
  int getNumberOfKeyFrames() const;

  // Returns a list of the keyframes.
  QList<vtkSMProxy*> getKeyFrames() const;

  // Insert a new keyframe at the given index.
  // The time for the key frame is computed using the times
  // for the neighbouring keyframes if any.
  // Returns the newly created keyframe proxy on success,
  // NULL otherwise.
  vtkSMProxy* insertKeyFrame(int index);

  // Deletes the keyframe at the given index.
  // Consequently, the keyframesModified() signal will get fired.
  void deleteKeyFrame(int index);

  // Returns keyframe at a given index, if one exists,
  // NULL otherwise.
  vtkSMProxy* getKeyFrame(int index) const;

  // Returns the animated proxy, if any.
  vtkSMProxy* getAnimatedProxy() const;

  // Returns the property that is animated by this cue, if any.
  vtkSMProperty* getAnimatedProperty() const;

  // Returns the index of the property being animated.
  int getAnimatedPropertyIndex() const;

  // Set the type of manipulator to create by default.
  void setManipulatorType(const QString& type)
    { this->ManipulatorType = type; }

  /// Set the type of the keyframe created by default.
  /// default is CompositeKeyFrame.
  void setKeyFrameType(const QString& type)
    { this->KeyFrameType = type; }

  // returns the manipulator proxy.
  vtkSMProxy* getManipulatorProxy() const;

  /// Sets default values for the underlying proxy. 
  /// This is during the initialization stage of the pqProxy 
  /// for proxies created by the GUI itself i.e.
  /// for proxies loaded through state or created by python client
  /// this method won't be called. 
  /// The default implementation iterates over all properties
  /// of the proxy and sets them to default values. 
  void setDefaultPropertyValues();

  /// Used by editors to trigger keyframesModified() signal after bulk of
  /// modifications have been made to the cue/key frames.
  void triggerKeyFramesModified()
    { emit this->keyframesModified(); }
   
  /// Get/Set the enabled state for the cue.
  void setEnabled(bool enable);
  bool isEnabled() const;

signals:
  // emitted when something about the keyframes changes.
  void keyframesModified();

  // Fired when the animated proxy/property/index 
  // changes.
  void modified();

  /// Fired when the enabled-state of the cue changes.
  void enabled(bool);

private slots:
  /// Called when the "Manipulator" property is changed.
  void onManipulatorModified();

  /// Called when the "Enabled" property is changed.
  void onEnabledModified();

private:
  pqAnimationCue(const pqAnimationCue&); // Not implemented.
  void operator=(const pqAnimationCue&); // Not implemented.

  /// Methods used to register/unregister keyframe proxies.
  void addKeyFrameInternal(vtkSMProxy*);
  void removeKeyFrameInternal(vtkSMProxy*);

  QString KeyFrameType;
  QString ManipulatorType;
  class pqInternals;
  pqInternals* Internal;
};


#endif

