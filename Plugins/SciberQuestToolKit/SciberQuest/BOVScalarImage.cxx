/*
 * Copyright 2012 SciberQuest Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither name of SciberQuest Inc. nor the names of any contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "BOVScalarImage.h"

#include "SQMacros.h"

//-----------------------------------------------------------------------------
MPI_File Open(MPI_Comm comm, MPI_Info hints, const char *fileName, int mode)
{
  #ifdef SQTK_WITHOUT_MPI
  (void)comm;
  (void)hints;
  (void)fileName;
  (void)mode;
  return 0;
  #else
  // added this to deal with vpic data arrays which use spaces.
  std::string cleanFileName=fileName;
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
    sqErrorMacro(std::cerr,
        << "Error opeing file: " << cleanFileName << std::endl
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
std::ostream &operator<<(std::ostream &os, const BOVScalarImage &si)
{
  os << si.GetName() << std::endl
     << "  " << si.GetFileName() << " " << si.GetFile() << std::endl;

  #ifndef SQTK_WITHOUT_MPI
  MPI_File file=si.GetFile();
  if (file)
    {
    os << "  Hints:" << std::endl;

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

        os << "    " << key << "=" << val << std::endl;
        }
      }
    }
  #endif

  return os;
}
