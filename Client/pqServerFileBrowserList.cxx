#include "pqServerFileBrowserList.h"

#include <vtkProcessModule.h>
#include <vtkClientServerStream.h>

#include <qfile.h>
#include <qfileinfo.h>
#include <qpixmap.h>
#include <qevent.h>
#include <qpoint.h>
#include <qmessagebox.h>
#include <q3dragobject.h>
#include <qmime.h>
#include <Q3StrList>
#include <qstringlist.h>
#include <qapplication.h>
#include <Q3Header>

static const char* folder_closed_xpm[]={
    "16 16 9 1",
    "g c #808080",
    "b c #c0c000",
    "e c #c0c0c0",
    "# c #000000",
    "c c #ffff00",
    ". c None",
    "a c #585858",
    "f c #a0a0a4",
    "d c #ffffff",
    "..###...........",
    ".#abc##.........",
    ".#daabc#####....",
    ".#ddeaabbccc#...",
    ".#dedeeabbbba...",
    ".#edeeeeaaaab#..",
    ".#deeeeeeefe#ba.",
    ".#eeeeeeefef#ba.",
    ".#eeeeeefeff#ba.",
    ".#eeeeefefff#ba.",
    ".##geefeffff#ba.",
    "...##gefffff#ba.",
    ".....##fffff#ba.",
    ".......##fff#b##",
    ".........##f#b##",
    "...........####."};

static const char* folder_open_xpm[]={
    "16 16 11 1",
    "# c #000000",
    "g c #c0c0c0",
    "e c #303030",
    "a c #ffa858",
    "b c #808080",
    "d c #a0a0a4",
    "f c #585858",
    "c c #ffdca8",
    "h c #dcdcdc",
    "i c #ffffff",
    ". c None",
    "....###.........",
    "....#ab##.......",
    "....#acab####...",
    "###.#acccccca#..",
    "#ddefaaaccccca#.",
    "#bdddbaaaacccab#",
    ".eddddbbaaaacab#",
    ".#bddggdbbaaaab#",
    "..edgdggggbbaab#",
    "..#bgggghghdaab#",
    "...ebhggghicfab#",
    "....#edhhiiidab#",
    "......#egiiicfb#",
    "........#egiibb#",
    "..........#egib#",
    "............#ee#"};

static const char * folder_locked[]={
    "16 16 10 1",
    "h c #808080",
    "b c #ffa858",
    "f c #c0c0c0",
    "e c #c05800",
    "# c #000000",
    "c c #ffdca8",
    ". c None",
    "a c #585858",
    "g c #a0a0a4",
    "d c #ffffff",
    "..#a#...........",
    ".#abc####.......",
    ".#daa#eee#......",
    ".#ddf#e##b#.....",
    ".#dfd#e#bcb##...",
    ".#fdccc#daaab#..",
    ".#dfbbbccgfg#ba.",
    ".#ffb#ebbfgg#ba.",
    ".#ffbbe#bggg#ba.",
    ".#fffbbebggg#ba.",
    ".##hf#ebbggg#ba.",
    "...###e#gggg#ba.",
    ".....#e#gggg#ba.",
    "......###ggg#b##",
    ".........##g#b##",
    "...........####."};

static const char * pix_file []={
    "16 16 7 1",
    "# c #000000",
    "b c #ffffff",
    "e c #000000",
    "d c #404000",
    "c c #c0c000",
    "a c #ffffc0",
    ". c None",
    "................",
    ".........#......",
    "......#.#a##....",
    ".....#b#bbba##..",
    "....#b#bbbabbb#.",
    "...#b#bba##bb#..",
    "..#b#abb#bb##...",
    ".#a#aab#bbbab##.",
    "#a#aaa#bcbbbbbb#",
    "#ccdc#bcbbcbbb#.",
    ".##c#bcbbcabb#..",
    "...#acbacbbbe...",
    "..#aaaacaba#....",
    "...##aaaaa#.....",
    ".....##aa#......",
    ".......##......."};

QPixmap *folderLocked = 0;
QPixmap *folderClosed = 0;
QPixmap *folderOpen = 0;
QPixmap *fileNormal = 0;

class pqFileItem : public Q3ListViewItem
{
public:
  pqFileItem( Q3ListViewItem *parent, const QString &s1, const QString &s2 )
  : Q3ListViewItem( parent, s1, s2 ), pix( 0 ) {}

  const QPixmap *pixmap( int i ) const;
  void setPixmap( QPixmap *p );

  int rtti() const;
  static int RTTI;

private:
  QPixmap *pix;

};

class pqDirectoryItem : public Q3ListViewItem
{
public:
  pqDirectoryItem( vtkProcessModule *module,vtkClientServerID serverFileListingID,pqDirectoryItem * parent, const QString& filename );
  pqDirectoryItem(vtkProcessModule *module,vtkClientServerID serverFileListingID, Q3ListView * parent, const QString& filename );

  QString text( int column ) const;

  QString fullName();

  void setOpen( bool );
  void setup();

  const QPixmap *pixmap( int i ) const;
  void setPixmap( QPixmap *p );


  int rtti() const;
  static int RTTI;


private:
  vtkProcessModule *mProcessModule;
  vtkClientServerID mServerFileListingID ;

  pqDirectoryItem * p;

  QPixmap *pix;
  QString f;
};

pqDirectoryItem::pqDirectoryItem( vtkProcessModule *module,vtkClientServerID serverFileListingID,pqDirectoryItem* parent, const QString& filename )
: Q3ListViewItem( parent ), f(filename),pix( 0 )
{
  p = parent;

  this->mProcessModule=module;
  mServerFileListingID=serverFileListingID;

  setPixmap( folderClosed );
}


pqDirectoryItem::pqDirectoryItem( vtkProcessModule *module,vtkClientServerID serverFileListingID,Q3ListView * parent, const QString& filename )
: Q3ListViewItem( parent ), f(filename),pix( 0 )
{
  p = 0;
  this->mProcessModule=module;
  mServerFileListingID=serverFileListingID;

}



void pqDirectoryItem::setPixmap( QPixmap *px )
{
  pix = px;
  setup();
  widthChanged( 0 );
  invalidateHeight();
  repaint();
}


const QPixmap *pqDirectoryItem::pixmap( int i ) const
{
  if ( i )
    return 0;
  return pix;
}

void pqDirectoryItem::setOpen( bool o )
{
  if ( o )
    setPixmap( folderOpen );
  else
    setPixmap( folderClosed );
  if(!this->mProcessModule)
  {
    QMessageBox mb("Prism",
      "Process Module not specified",
      QMessageBox::Critical,
      QMessageBox::Ok,
      QMessageBox::NoButton,
      QMessageBox::NoButton);

    mb.exec();
    return;

  }

  if ( o && !childCount() )
  {
    QString s( fullName() );

    listView()->setUpdatesEnabled( FALSE );

    vtkClientServerStream stream;

    stream << vtkClientServerStream::Invoke
      << mServerFileListingID << "GetFileListing" << s.ascii() << 0
      << vtkClientServerStream::End;

    this->mProcessModule->SendStream(vtkProcessModule::DATA_SERVER_ROOT, stream);

    vtkClientServerStream result;
    if(!this->mProcessModule->GetLastResult(vtkProcessModule::DATA_SERVER_ROOT).GetArgument(0, 0, &result))
    {
    }

    if(result.GetNumberOfMessages() == 2)
    {
      int i;
      // The first message lists directories.
      for(i=0; i < result.GetNumberOfArguments(0); ++i)
      {
        const char* d;
        if(result.GetArgument(0, i, &d))
        {
          QString name(d);
          (void)new pqDirectoryItem(this->mProcessModule,mServerFileListingID, this,name );
        }
      }

      // The second message lists files.
      for(i=0; i < result.GetNumberOfArguments(1); ++i)
      {
        const char* f;
        if(result.GetArgument(1, i, &f))
        {
          QString name(f);
          pqFileItem *item = new pqFileItem( this, name,"File");
          item->setPixmap( fileNormal );
        }
      }
    }
    listView()->setUpdatesEnabled( TRUE );
  }
  Q3ListViewItem::setOpen( o );
}


void pqDirectoryItem::setup()
{
  setExpandable( TRUE );
  Q3ListViewItem::setup();
}


QString pqDirectoryItem::fullName()
{
  QString s;
  if ( p ) 
  {
    s = p->fullName();
    s.append( f);
    s.append( "/" );
  } else
  {
    s = f;
    s.append( "/" ); 
  }
  return s;
}


QString pqDirectoryItem::text( int column ) const
{
  if ( column == 0 )
    return f;

    
  return "Directory";
 }
int pqDirectoryItem::RTTI = 7777;

/* \reimp */

int pqDirectoryItem::rtti() const
{
  return RTTI;
}

pqServerFileBrowserList::pqServerFileBrowserList(QWidget *parent, const char *name )
: Q3ListView( parent, name )
{
  this->mProcessModule = 0;
  
  autoopen_timer = new QTimer( this );
  if ( !folderLocked ) {
    folderLocked = new QPixmap( folder_locked );
    folderClosed = new QPixmap( folder_closed_xpm );
    folderOpen = new QPixmap( folder_open_xpm );
    fileNormal = new QPixmap( pix_file );
  }

  connect( this, SIGNAL( doubleClicked( Q3ListViewItem * ) ),
    this, SLOT( slotFolderSelected( Q3ListViewItem * ) ) );
  connect( this, SIGNAL( returnPressed( Q3ListViewItem * ) ),
    this, SLOT( slotFolderSelected( Q3ListViewItem * ) ) );

  setAcceptDrops( TRUE );
  viewport()->setAcceptDrops( TRUE );

  connect( autoopen_timer, SIGNAL( timeout() ),
    this, SLOT( openFolder() ) );


  this->addColumn( "Name" );
  this->addColumn( "Type" );
  this->setTreeStepSize( 20 );

  this->setAllColumnsShowFocus( TRUE );
  this->show();
}

void pqServerFileBrowserList::setProcessModule(vtkProcessModule *module)
{
  vtkClientServerStream stream;
  if(this->mProcessModule)
  {
    this->mProcessModule->DeleteStreamObject(mServerFileListingID, stream);
    this->mProcessModule->SendStream(vtkProcessModule::DATA_SERVER_ROOT, stream);
  }

  this->mProcessModule=module;

  mServerFileListingID = this->mProcessModule->NewStreamObject("vtkPVServerFileListing", stream);

  stream << vtkClientServerStream::Invoke
    << mServerFileListingID << "GetCurrentWorkingDirectory"
    << vtkClientServerStream::End;
  this->mProcessModule->SendStream(vtkProcessModule::DATA_SERVER_ROOT, stream);
  const char* cwd;
  if(!this->mProcessModule->GetLastResult(
    vtkProcessModule::DATA_SERVER_ROOT).GetArgument(0, 0, &cwd))
  {
    cwd = "";
  }

  mHome=cwd;

  this->setDir(mHome);

}

pqServerFileBrowserList::~pqServerFileBrowserList()
{
  vtkClientServerStream stream;

  if(this->mProcessModule)
  {
    this->mProcessModule->DeleteStreamObject(mServerFileListingID, stream);
    this->mProcessModule->SendStream(vtkProcessModule::DATA_SERVER_ROOT, stream);
  }
}

void pqServerFileBrowserList::slotFolderSelected( Q3ListViewItem *i )
{
  if ( !i )
    return;

  if(i->rtti()==pqFileItem::RTTI)
  {
    pqFileItem *file = (pqFileItem*)i;
    QString name=fullPath(file);
    emit fileSelected(name );
  }
}

void pqServerFileBrowserList::openFolder()
{
  autoopen_timer->stop();
}


void pqServerFileBrowserList::upDir()
{
  clear();
  QString path=mCurrent;
  int index=path.findRev("/");
  if(index==-1)
  {
    index=path.findRev("\\");
  
  }

  if(index==-1)
  {
    setDir(mCurrent);
    return;
  }
  path.truncate(index);
  setDir(path);

}

void pqServerFileBrowserList::home()
{
  clear();
  setDir(mHome);
}

static const int autoopenTime = 750;




QString pqServerFileBrowserList::fullPath(Q3ListViewItem* item)
{
  QString fullpath = item->text(0);
  while ( (item=item->parent()) )
  {
    fullpath = item->text(0) + "/" + fullpath;
  }

  return fullpath;
}


void pqServerFileBrowserList::setDir(QString &s )
{
  clear();
  mCurrent=s;
  pqDirectoryItem* root = new pqDirectoryItem(this->mProcessModule,mServerFileListingID,this,s);

  root->setOpen(TRUE);
}

void pqFileItem::setPixmap( QPixmap *p )
{
  pix = p;
  setup();
  widthChanged( 0 );
  invalidateHeight();
  repaint();
}


const QPixmap *pqFileItem::pixmap( int i ) const
{
  if ( i )
    return 0;
  return pix;
}
int pqFileItem::RTTI = 7778;

/* \reimp */

int pqFileItem::rtti() const
{
  return RTTI;
}
