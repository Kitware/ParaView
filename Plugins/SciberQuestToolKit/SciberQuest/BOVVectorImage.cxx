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
#include "BOVVectorImage.h"

//-----------------------------------------------------------------------------
BOVVectorImage::~BOVVectorImage()
{
  this->Clear();
}

//-----------------------------------------------------------------------------
void BOVVectorImage::Clear()
{
  int nComps=(int)this->ComponentFiles.size();
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
        const char *fileName,
        int mode)
{
  BOVScalarImage *oldComp = this->ComponentFiles[i];

  if (oldComp)
    {
    delete oldComp;
    }

  this->ComponentFiles[i] = new BOVScalarImage(comm,hints,fileName,mode);
}

//-----------------------------------------------------------------------------
void BOVVectorImage::SetNumberOfComponents(int nComps)
{
  this->Clear();
  this->ComponentFiles.resize(nComps,0);
}

//-----------------------------------------------------------------------------
std::ostream &operator<<(std::ostream &os, const BOVVectorImage &vi)
{
  os << vi.GetName() << std::endl;

  int nComps = vi.GetNumberOfComponents();
  for (int i=0; i<nComps; ++i)
    {
    os
      << "    " << vi.ComponentFiles[i]->GetFileName()
      << " "    << vi.ComponentFiles[i]->GetFile()
      << std::endl;
    }

  #ifndef SQTK_WITHOUT_MPI
  // only one of the file's hints
  MPI_File file=vi.ComponentFiles[0]->GetFile();
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
