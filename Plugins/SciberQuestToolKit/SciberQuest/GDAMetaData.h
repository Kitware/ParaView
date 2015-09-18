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
#ifndef GDAMetaData_h
#define GDAMetaData_h

#include "BOVMetaData.h"

#include <ostream> // for ostream

/// Parser for SciberQuest GDA dataset format.
/**
Parser for SciberQuest's GDA dataset metadata format.
*/
class GDAMetaData : public BOVMetaData
{
public:
  /// Constructors.
  GDAMetaData();
  GDAMetaData(const GDAMetaData &other) : BOVMetaData(other)
    {
    *this=other;
    }

  /// Assignment.
  GDAMetaData &operator=(const GDAMetaData &other);

  /// Destructor.
  virtual ~GDAMetaData()
    {
    this->CloseDataset();
    }

  /**
  Virtual copy constructor. Create a new object and copy. return the copy.
  or 0 on error. Caller to delete.
  */
  virtual BOVMetaData *Duplicate() const
    {
    GDAMetaData *other=new GDAMetaData;
    *other=*this;
    return other;
    }

  /**
  Open the metadata file, and parse metadata.
  return 0 on error.
  */
  virtual int OpenDataset(const char *fileName, char mode);

  /**
  Free any resources and set the object into a default
  state.
  return 0 on error.
  */
  virtual int CloseDataset();

  /**
  Write the object state in the metadata format. return 0 on error.
  */
  virtual int Write();

  /**
  Return the file extension used by metadata files.
  */
  //virtual const char *GetMetadataFileExtension() const =0;

  /// Add our keys to the pipeline information.
  virtual void PushPipelineInformation(
        vtkInformation *req,
        vtkInformation *pinfo);

  /// Print internal state.
  virtual void Print(std::ostream &os) const;

private:
  void ClearCoordinates();
  int OpenDatasetForRead(const char *fileName);
  int OpenDatasetForWrite(const char *fileName, char mode);

private:
  bool HasDipoleCenter;
  double DipoleCenter[3];
};

#endif

// VTK-HeaderTest-Exclude: GDAMetaData.h
