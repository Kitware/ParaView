/*=========================================================================

   Program: ParaView
   Module:    pqPVNewSourceBehavior.h

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
#ifndef __pqPVNewSourceBehavior_h 
#define __pqPVNewSourceBehavior_h

#include <QObject>
#include "pqApplicationComponentsExport.h"

class pqProxy;

/// @ingroup Behaviors
/// ParaView has quite a few peculiar activities that it likes to do when a new
/// source/filter is created e.g.
/// \li The new source is made active.
/// \li If the new source is a temporal filter, ensure that the timesteps
///     provided by the input are not considered in the animation.
///
/// All these are managed by this class. Note that this class performs any tasks
/// only when the source was created using the pqObjectBuilder.
class PQAPPLICATIONCOMPONENTS_EXPORT pqPVNewSourceBehavior : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;
public:
  pqPVNewSourceBehavior(QObject* parent=0);

protected slots:
  void activate(pqProxy*);

private:
  Q_DISABLE_COPY(pqPVNewSourceBehavior)
};

#endif


