/*=========================================================================

   Program:   ParaView
   Module:    pqSignalAdaptorKeyFrameValue.h

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
#ifndef __pqSignalAdaptorKeyFrameValue_h
#define __pqSignalAdaptorKeyFrameValue_h

#include "pqComponentsExport.h"
#include <QObject>
#include <QList>
#include <QVariant>

class vtkSMProxy;
class pqAnimationCue;

// pqSignalAdaptorKeyFrameValue is a special signal adaptor used to
// link the GUI widget showing the "KeyValue" of the key frame
// and the SMProperty on the key frame proxy. This adaptor
// itself manages the type of widget used for the value depending
// on the type of animated property, hence typically
// the application simply creates an empty container frame 
// to contain the value widget (passed as \c parent to the constructor).
class PQCOMPONENTS_EXPORT pqSignalAdaptorKeyFrameValue : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QList<QVariant> values READ values WRITE setValue)
  Q_PROPERTY(QVariant value READ value WRITE setValue)
public:
  // Constructor. 
  // \c lparent is the frame which can be used to pack large widgets
  // such as Contour Values editor widgets, while \c parent 
  // is the frame in which small widgets such as check box etc are packed.
  pqSignalAdaptorKeyFrameValue(QWidget* lparent, QWidget* parent);
  virtual ~pqSignalAdaptorKeyFrameValue();

  // Get/Set the animation cue on which we've added the key frame
  // being added. This is needed since this class needs to know the
  // type of property being animated to determine the type of
  // widget to create for the changing the value.
  void setAnimationCue(pqAnimationCue* cue);
  pqAnimationCue* getAnimationCue() const;

  // Get the value as string. This is the value for the 
  // "KeyValue" property of the KeyFrame proxy.
  QList<QVariant> values() const;

  QVariant value() const;

  // Attempts to set the value to the minimum specified by the domain.
  void setValueToMin() { this->onMin(); }
  
  // Attempts to set the value to the maximum specified by the domain.
  void setValueToMax() { this->onMax(); }

  // Attempts to set the value using the current value of the 
  // animated property.
  void setValueToCurrent();
signals:
  // Fired when the value changes.
  void valueChanged();

public slots:
  // Set the value as string. This is the value for the 
  // "KeyValue" property of the KeyFrame proxy.
  void setValue(const QList<QVariant>&);
  void setValue(QVariant v);
    
private slots:
  // Called when animation cue is modified.
  void onCueModified();

  // Called when the domain for the animated property changes.
  // We decide whether to show the min/max buttons,
  // update domain depenedent GUI etc.
  void onDomainChanged();

  void onMin();
  void onMax();
private:
  pqSignalAdaptorKeyFrameValue(const pqSignalAdaptorKeyFrameValue&); // Not implemented.
  void operator=(const pqSignalAdaptorKeyFrameValue&); // Not implemented.

  class pqInternals;
  pqInternals* Internals;
};

#endif

