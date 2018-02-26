/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

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
#ifndef PQCUSTOMSPLITTER_H
#define PQCUSTOMSPLITTER_H

#include "pqComponentsModule.h"

#include <QPainter>
#include <QSplitter>

/**
 * @class pqSplitter
 * @brief A custom splitter derived from QSplitter.
 *
 * It is used to adjust the separator width and color displayed in the
 * preview mode based the user inputs when a SaveScreenshot or SaveAnimation
 * dialog is active. Since the handlewidth is set to 0 in the preview mode,
 * pqSplitter must be able to draw a separator whose width is not equal
 * to the handle width. This is implemented through a custom splitter handle
 * which overrides the paintEvent such that a rectangular separator with
 * adjustable width and color is painted regardless of the handle width.
 */
class PQCOMPONENTS_EXPORT pqSplitter : public QSplitter
{
  Q_OBJECT
public:
  explicit pqSplitter(QWidget* parent = Q_NULLPTR)
    : QSplitter(parent)
  {
  }
  ~pqSplitter() {}
  /**
  * Set whether a separator should be painted or not.
  */
  void setSplitterHandlePaint(bool);
  /**
  * Set the width of the separator to be painted.
  */
  void setSplitterHandleWidth(int);
  /**
  * Set the color of the separator to be painted.
  */
  void setSplitterHandleColor(double[3]);

private:
  class pqSplitterHandle;

protected:
  /**
  * Create a pqSplitterHandle.
  */
  QSplitterHandle* createHandle() override;
  /**
  * Return the pqSplitterHandle at specific index.
  */
  pqSplitterHandle* handle(int) const;
};

#endif // PQCUSTOMSPLITTER_H
