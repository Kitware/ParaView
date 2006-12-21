/*=========================================================================

   Program: ParaView
   Module:    pqSourceInfoIcons.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

/// \file pqSourceInfoIcons.h
/// \date 6/9/2006

#ifndef _pqSourceInfoIcons_h
#define _pqSourceInfoIcons_h


#include "pqComponentsExport.h"
#include <QObject>
#include <QPixmap> // Needed for return value

class pqSourceInfoIconsInternal;
class QString;


/// \class pqSourceInfoIcons
/// \brief
///   The pqSourceInfoIcons class makes it possible to associate a
///   pixmap with a specific source.
class PQCOMPONENTS_EXPORT pqSourceInfoIcons : public QObject
{
  Q_OBJECT

public:
  enum DefaultPixmap
    {
    Invalid = -1,
    Server = 0,
    Source,
    Reader,
    Filter
    };

public:
  pqSourceInfoIcons(QObject *parent=0);
  virtual ~pqSourceInfoIcons();

  /// \brief
  ///   Gets the default pixmap for the given type.
  /// \param type The default pixmap type.
  /// \return
  ///   The default pixmap for the given type.
  /// \sa pqSourceInfoIcons::getPixmap(const QString &, DefaultPixmap)
  QPixmap getDefaultPixmap(DefaultPixmap type) const;

  /// \brief
  ///   Gets the pixmap associated with the specified source.
  /// \param source The name of the source.
  /// \param alternate The pixmap to use if the source doesn't have an
  ///   associated pixmap.
  /// \return
  ///   The pixmap associated with the specified source.
  QPixmap getPixmap(const QString &source, DefaultPixmap alternate) const;

  /// \brief
  ///   Associates a pixmap with the specified source.
  ///
  /// If the source already has an assigned pixmap, the old one is
  /// replaced with the new one.
  ///
  /// \param source The name of the source.
  /// \param fileName The pixmap file name or resource path.
  void setPixmap(const QString &source, const QString &fileName);

  /// Clears all the source to icon mappings.
  void clearPixmaps();

signals:
  /// \brief
  ///   Emitted when a special pixmap is assigned to source.
  /// \param name The name of the source.
  void pixmapChanged(const QString &name);

private:
  pqSourceInfoIconsInternal *Internal; ///< Maps source name to icon.
};

#endif
