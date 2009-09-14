/*=========================================================================

   Program: ParaView
   Module:    pqProgressBarHelper.h

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

#ifndef PQ_PROGESS_BAR_HELPER_HPP
#define PQ_PROGESS_BAR_HELPER_HPP

#include <QWidget>
class pqProgressBar;
class QProgressBar;

// progress bar helper class
// we can't call QCoreApplication::processEvents to update the progress bar
// so we'll do some extra work to make the progress bar update
#ifdef Q_WS_MAC
class pqProgressBarHelper : public QWidget
#else
class pqProgressBarHelper : public QObject
#endif
{
public:
  pqProgressBarHelper(pqProgressBar* p);
  
  void setFormat(const QString& fmt);

  void setProgress(int num);
  
  void enableProgress(bool e);

  bool progressEnabled() const;

  pqProgressBar* ParentProgress;
#ifdef Q_WS_MAC
  QProgressBar* Progress;
#endif
};

#endif


