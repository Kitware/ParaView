/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#include "BOVVectorImage.h"

//-----------------------------------------------------------------------------
BOVVectorImage::~BOVVectorImage()
{
  this->Clear();
}

//-----------------------------------------------------------------------------
void BOVVectorImage::Clear()
{
  int nComps=this->ComponentFiles.size();
  for (int i=0; i<nComps; ++i)
    {
    BOVScalarImage *comp=this->ComponentFiles[i];
    if (comp)
      {
      delete comp;
      }
    }
  this->ComponentFiles.clear();
}

//-----------------------------------------------------------------------------
void BOVVectorImage::SetComponentFile(
        int i,
        MPI_Comm comm,
        MPI_Info hints,
        const char *fileName)
{
  BOVScalarImage *oldComp = this->ComponentFiles[i];

  if (oldComp)
    {
    delete oldComp;
    }

  this->ComponentFiles[i] = new BOVScalarImage(comm,hints,fileName);
}

//-----------------------------------------------------------------------------
void BOVVectorImage::SetNumberOfComponents(int nComps)
{
  this->Clear();
  this->ComponentFiles.resize(nComps,0);
}

//-----------------------------------------------------------------------------
ostream &operator<<(ostream &os, const BOVVectorImage &vi)
{
  os << vi.GetName() << endl;

  int nComps = vi.GetNumberOfComponents();
  for (int i=0; i<nComps; ++i)
    {
    os
      << "    " << vi.ComponentFiles[i]->GetFileName()
      << " "    << vi.ComponentFiles[i]->GetFile()
      << endl;
    }

  // only one of the file's hints
  MPI_File file=vi.ComponentFiles[0]->GetFile();
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
