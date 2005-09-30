/*=========================================================================

  Program:   Qt & ParaView
  Module:    pqServerFileBrowserList.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright 2004 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
  license for use of this work by or on behalf of the
  U.S. Government. Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that this Notice and any
  statement of authorship are reproduced on all copies.

=========================================================================*/
//Patterned after the Qt example DirView

#ifndef _pqServerFileBrowserListList_h
#define _pqServerFileBrowserListList_h

#include <QColorGroup>
#include <Q3ListView>
#include <QString>
#include <QFile>
#include <QFileInfo>
#include <QTimer>

#include <vtkClientServerStream.h>
class vtkProcessModule;
class QWidget;

class pqServerFileBrowserList : public Q3ListView
{
    Q_OBJECT

public:
    pqServerFileBrowserList(QWidget *parent = 0, const char *name = 0 );
    ~pqServerFileBrowserList();
    
    void setProcessModule(vtkProcessModule *module);
    QString getCurrentPath();

signals:
    void fileSelected( const QString & );

public slots:
    void upDir();
    void home();

protected slots:
    void slotFolderSelected( Q3ListViewItem * );
    void openFolder();

private:
    QString fullPath(Q3ListViewItem* item);
    void setDir(QString & ); 
    
    vtkProcessModule *mProcessModule;
    vtkClientServerID mServerFileListingID ;

    QTimer* autoopen_timer;
    QString mHome;
    QString mCurrent;
};

#endif
