/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#include "BOVScalarImage.h"

#include "SQMacros.h"

//-----------------------------------------------------------------------------
MPI_File Open(MPI_Comm comm, MPI_Info hints, const char *fileName, int mode)
{
  #ifdef SQTK_WITHOUT_MPI
  return 0;
  #else
  // added this to deal with vpic data arrays which use spaces.
  string cleanFileName=fileName;
  size_t fileNameLen=cleanFileName.size();
  for (size_t i=0; i<fileNameLen; ++i)
    { 
    if (cleanFileName[i]==' ') cleanFileName[i]='-';
    }

  MPI_File file=0;
  int iErr;
  const int eStrLen=2048;
  char eStr[eStrLen]={'\0'};
  // Open the file
  iErr=MPI_File_open(
      comm,
      const_cast<char *>(cleanFileName.c_str()),
      mode,
      hints,
      &file);
  if (iErr!=MPI_SUCCESS)
    {
    MPI_Error_string(iErr,eStr,const_cast<int *>(&eStrLen));
    sqErrorMacro(cerr,
        << "Error opeing file: " << cleanFileName << endl
        << eStr);
    file=0;
    }
  return file;
  #endif
}

//-----------------------------------------------------------------------------
BOVScalarImage::BOVScalarImage(
    MPI_Comm comm,
    MPI_Info hints,
    const char *fileName,
    int mode)
{
  this->File=Open(comm,hints,fileName,mode);
  this->FileName=fileName;
}

//-----------------------------------------------------------------------------
BOVScalarImage::BOVScalarImage(
      MPI_Comm comm,
      MPI_Info hints,
      const char *fileName,
      const char *name,
      int mode)
{
  this->File=Open(comm,hints,fileName,mode);
  this->FileName=fileName;
  this->Name=name;
}

//-----------------------------------------------------------------------------
BOVScalarImage::~BOVScalarImage()
{
  #ifndef SQTK_WITHOUT_MPI
  if (this->File)
    {
    MPI_File_close(&this->File);
    }
  #endif
}

//-----------------------------------------------------------------------------
ostream &operator<<(ostream &os, const BOVScalarImage &si)
{
  os << si.GetName() << endl
     << "  " << si.GetFileName() << " " << si.GetFile() << endl;

  #ifndef SQTK_WITHOUT_MPI
  MPI_File file=si.GetFile();
  if (file)
    {
    os << "  Hints:" << endl;

    int WorldRank;
    MPI_Comm_rank(MPI_COMM_WORLD,&WorldRank);
    if (WorldRank==0)
      {
      MPI_Info info;
      char key[MPI_MAX_INFO_KEY];
      char val[MPI_MAX_INFO_KEY];
      MPI_File_get_info(file,&info);
      int nKeys;
      MPI_Info_get_nkeys(info,&nKeys);
      for (int i=0; i<nKeys; ++i)
        {
        int flag;
        MPI_Info_get_nthkey(info,i,key);
        MPI_Info_get(info,key,MPI_MAX_INFO_KEY,val,&flag);

        os << "    " << key << "=" << val << endl;
        }
      }
    }
  #endif

  return os;
}
