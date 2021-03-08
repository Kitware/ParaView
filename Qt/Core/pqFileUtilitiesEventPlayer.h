/*=========================================================================

   Program: ParaView
   Module:  pqFileUtilitiesEventPlayer.h

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
#ifndef pqFileUtilitiesEventPlayer_h
#define pqFileUtilitiesEventPlayer_h

#include "pqCoreModule.h" // for exports.
#include <pqWidgetEventPlayer.h>

/**
 * @class pqFileUtilitiesEventPlayer
 * @brief adds support for miscellaneous file-system utilities for test playback
 *
 * pqFileUtilitiesEventPlayer is a pqWidgetEventPlayer subclass that adds
 * support for a set of commands that help with file system checks and changes
 * e.g. testing for file, removing file, copying file etc.
 */
class PQCORE_EXPORT pqFileUtilitiesEventPlayer : public pqWidgetEventPlayer
{
  Q_OBJECT
  typedef pqWidgetEventPlayer Superclass;

public:
  pqFileUtilitiesEventPlayer(QObject* parent = nullptr);

  using Superclass::playEvent;
  bool playEvent(
    QObject* qobject, const QString& command, const QString& args, bool& errorFlag) override;

private:
  Q_DISABLE_COPY(pqFileUtilitiesEventPlayer);
};

#endif
