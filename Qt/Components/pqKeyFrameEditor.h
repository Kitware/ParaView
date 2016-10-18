/*=========================================================================

   Program:   ParaQ
   Module:    pqKeyFrameEditor.h

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.2.

   See License_v1.2.txt for the full ParaQ license.
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

#ifndef _pqKeyFrameEditor_h
#define _pqKeyFrameEditor_h

#include "pqComponentsModule.h"
#include <QDialog>
#include <QWidget>

class pqAnimationCue;
class pqAnimationScene;

/**
* editor for editing animation key frames
*/
class PQCOMPONENTS_EXPORT pqKeyFrameEditor : public QWidget
{
  typedef QWidget Superclass;

  Q_OBJECT

public:
  pqKeyFrameEditor(pqAnimationScene* scene, pqAnimationCue* cue, const QString& label, QWidget* p);
  ~pqKeyFrameEditor();

  /**
  * The keyframe editor can be set in a mode where the user can only edit the
  * key frame values or keyframe interpolation and not add/delete keyframes
  * or change key time. To enable this mode, set this to true (false by
  * default).
  */
  void setValuesOnly(bool);
  bool valuesOnly() const;

public slots:
  /**
  * read the key frame data and display it
  */
  void readKeyFrameData();
  /**
  * write the key frame data as edited by the user to the server manager
  */
  void writeKeyFrameData();

private slots:
  void newKeyFrame();
  void deleteKeyFrame();
  void deleteAllKeyFrames();
  void useCurrentCamera(QObject*);

private:
  class pqInternal;
  pqInternal* Internal;
};

// internal class
class pqKeyFrameEditorDialog : public QDialog
{
  Q_OBJECT
public:
  pqKeyFrameEditorDialog(QWidget* p, QWidget* child);
  ~pqKeyFrameEditorDialog();
  QWidget* Child;
};

#endif
