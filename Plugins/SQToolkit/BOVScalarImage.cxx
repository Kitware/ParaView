/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2008 SciberQuest Inc.
*/
#include "BOVScalarImage.h"

#include "SQMacros.h"

//-----------------------------------------------------------------------------
MPI_File Open(MPI_Comm comm, MPI_Info hints, const char *fileName)
{
  MPI_File file=0;
  int iErr;
  const int eStrLen=2048;
  char eStr[eStrLen]={'\0'};
  // Open the file
  iErr=MPI_File_open(
      comm,
      const_cast<char *>(fileName),
      MPI_MODE_RDONLY,
      hints,
      &file);
  if (iErr!=MPI_SUCCESS)
    {
    MPI_Error_string(iErr,eStr,const_cast<int *>(&eStrLen));
    sqErrorMacro(cerr,
        << "Error opeing file: " << fileName << endl
        << eStr);
    file=0;
    }
  return file;
}

//-----------------------------------------------------------------------------
BOVScalarImage::BOVScalarImage(
    MPI_Comm comm,
    MPI_Info hints,
    const char *fileName)
{
  this->File=Open(comm,hints,fileName);
  this->FileName=fileName;
}

//-----------------------------------------------------------------------------
BOVScalarImage::BOVScalarImage(
      MPI_Comm comm,
      MPI_Info hints,
      const char *fileName,
      const char *name)
{
  this->File=Open(comm,hints,fileName);
  this->FileName=fileName;
  this->Name=name;
}

//-----------------------------------------------------------------------------
BOVScalarImage::~BOVScalarImage()
{
  if (this->File)
    {
    MPI_File_close(&this->File);
    }
}

//-----------------------------------------------------------------------------
ostream &operator<<(ostream &os, const BOVScalarImage &si)
{
  os << si.GetName() << endl
     << "  " << si.GetFileName() << " " << si.GetFile() << endl;

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

  return os;
}
