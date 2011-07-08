/*   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#ifndef GDAMetaData_h
#define GDAMetaData_h

#include "SQExport.h"
#include "BOVMetaData.h"

#include <iostream>
using std::ostream;


/// Parser for SciberQuest GDA dataset format.
/**
Parser for SciberQuest's GDA dataset metadata format.
*/
class SQ_EXPORT GDAMetaData : public BOVMetaData
{
public:
  /// Constructors.
  GDAMetaData();
  GDAMetaData(const GDAMetaData &other)
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
  virtual int OpenDataset(const char *fileName);

  /**
  Free any resources and set the object into a default
  state.
  return 0 on error.
  */
  virtual int CloseDataset();

  /**
  Return the file extension used by the format for brick files.
  The BOV reader will make use of this in its pattern matching logic.
  */
  virtual const char *GetBrickFileExtension() const
    {
    return "gda";
    }

  /**
  Return the file extension used by metadata files.
  */
  //virtual const char *GetMetadataFileExtension() const =0;

  /// Add our keys to the pipeline information.
  virtual void PushPipelineInformation(
        vtkInformation *req,
        vtkInformation *pinfo);

  /// Print internal state.
  virtual void Print(ostream &os) const;

private:
  void ClearCoordinates();

private:
  bool HasDipoleCenter;
  double DipoleCenter[3];
};

#endif
