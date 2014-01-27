/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#ifndef BOVMetaData_h
#define BOVMetaData_h

#include "BinaryStream.hxx" // for BinaryStream
#include "CartesianExtent.h" // for CartesianExtent
#include "SharedArray.hxx" // for SharedArray

#include <cstdlib> // for size_t
#include <map> // for map
#include <vector> // for vector
#include <string> // for string

// These masks are used with array status methods.
// ACTIVE_BIT is set to indicate an array is to be read
// VECTOR_BIT is set to indicate an array is a vector, cleared for scalar.
#define ACTIVE_BIT       0x01
#define SCALAR_BIT       0x02
#define VECTOR_BIT       0x04
#define TENSOR_BIT       0x08
#define SYM_TENSOR_BIT   0x10
#define ARRAY_TYPE_BITS (SCALAR_BIT|VECTOR_BIT|TENSOR_BIT|SYM_TENSOR_BIT)

class vtkInformation;

/// Interface to a BOV MetaData file.
/**
This class defines the interface to a BOV MetaData file, implement
this interface and fill the data structures during open to enable the
BOVReader to read your dataset.
*/
class BOVMetaData
{
public:
  BOVMetaData();
  BOVMetaData(const BOVMetaData &other){ *this=other; }
  BOVMetaData &operator=(const BOVMetaData &other);
  virtual ~BOVMetaData();

  /**
  Virtual copy constructor. Create a new object and copy this into it.
  return the copy or 0 on error. Caller to delete.
  */
  virtual BOVMetaData *Duplicate() const=0;

  /// \Section FileOperations \@{
  /**
  Return the filename of the open dataset
  */
  const char *GetFileName() const { return this->FileName.c_str(); }

  /**
  Set/Get the directory where the bricks for th eopen dataset are
  located. Valid if a metadata file has been successfully opened.
  */
  void SetPathToBricks(const char *path) {  this->PathToBricks=path; }
  const char *GetPathToBricks() const { return this->PathToBricks.c_str(); }

  /**
  Return the file extension used by the format for brick files.
  The BOV reader will make use of this in its pattern matching logic.
  */
  void SetBrickFileExtension(std::string &ext){ this->BrickFileExtension=ext; }
  virtual const char *GetBrickFileExtension() const { return this->BrickFileExtension.c_str(); }

  /**
  Return the file extension used by metadata files.
  */
  //virtual const char *GetMetadataFileExtension() const =0;

  /**
  Open the metadata file. In read mode, 'r', parse metadata. In write
  mode, 'w', store the filename. return 0 on error.
  */
  virtual int OpenDataset(const char *fileName, char mode)=0;

  /**
  Return true if "Get" calls will succeed, i.e. there is an open metadata
  file.
  */
  virtual int IsDatasetOpen() const { return this->IsOpen; }

  /**
  Get the mode that the file was opened in.
  */
  virtual char GetMode() const { return this->Mode; }
  virtual bool ReadMode() const { return this->Mode=='r'; }
  virtual bool WriteMode() const { return (this->Mode=='w')||(this->Mode=='a'); }

  /**
  Write the object state in the metadata format. return 0 on error.
  */
  virtual int Write()=0;

  /**
  Close the currently open metatdata file, free any resources and set
  the object into a default state. return 0 on error. Be sure to call
  BOVMetaData::CloseDataset().
  */
  virtual int CloseDataset();
  /// \@}


  /// \Section DomainSpecification \@{
  /**
  Set/Get the domain/subset/decomp of the dataset. These are initialized
  during OpenDataset(). The subset may be modified anytime there after by the user
  to reduce the amount of memory used during the actual read. Definitions:
  The domain is index space of the data on disk, the subset is the index space
  user identifies to read from disk, and the decomp is the index space assigned
  to this process to read. Note: Setting the domain also sets the subset and
  decomp, and setting the subset also sets the decomp.
  */
  void SetDomain(const CartesianExtent &domain);
  void SetSubset(const CartesianExtent &subset);
  void SetDecomp(const CartesianExtent &decomp);

  CartesianExtent GetDomain() const { return this->Domain; }
  CartesianExtent GetSubset() const { return this->Subset; }
  CartesianExtent GetDecomp() const { return this->Decomp; }

  /**
  Return a string naming the vtk dataset that is to be used to
  hold the data. Possible values include vtkImageData, vtkRectilinearGrid,
  vtkStructuredGrid.
  */
  virtual const char *GetDataSetType() const { return this->DataSetType.c_str(); }
  virtual void SetDataSetType(const char *type){ this->DataSetType=type; }

  virtual bool DataSetTypeIsImage() const { return (this->DataSetType[3]=='I'); }
  virtual bool DataSetTypeIsRectilinear() const { return (this->DataSetType[3]=='R'); }
  virtual bool DataSetTypeIsStructured() const { return (this->DataSetType[3]=='S'); }

  /**
  Set/Get the dataset origin. Used only with vtkImageData.
  */
  void SetOrigin(const double *origin);
  void SetOrigin(double x0, double y0, double z0);
  void GetOrigin(double *origin) const;
  double *GetOrigin(){ return this->Origin; }
  const double *GetOrigin() const { return this->Origin; }

  /**
  Set/Get the grid spacing. Used only for vtkImageData.
  */
  void SetSpacing(const double *spacing);
  void SetSpacing(double dx, double dy, double dz);
  void GetSpacing(double *spacing) const;
  double *GetSpacing(){ return this->Spacing; }
  const double *GetSpacing() const { return this->Spacing; }

  /**
  Set/Get the time step
  */
  void SetDt(double dt) { this->Dt=dt; }
  double GetDt() const { return this->Dt; }

  /**
  Set/Get the qth domain coordinate array. Used only for vtkRectininearGrid.
  */
  virtual const SharedArray<float> *GetCoordinate(int q) const { return this->Coordinates[q]; }
  virtual SharedArray<float> *GetCoordinate(int q) { return this->Coordinates[q]; }
  virtual void AssignCoordinate(int q, float *coord, size_t n){ this->Coordinates[q]->Assign(coord,n); }
  virtual float *SubsetCoordinate(int q, CartesianExtent &ext) const;
  /// \@}

  /// \Section ArrayModifiers \@{
  /**
  Add an array to the list of available arrays.
  */
  void AddScalar(const char *name){ this->Arrays[name]=SCALAR_BIT; }
  void AddVector(const char *name){ this->Arrays[name]=VECTOR_BIT; }
  void AddTensor(const char *name){ this->Arrays[name]=TENSOR_BIT; }
  void AddSymetricTensor(const char *name){ this->Arrays[name]=SYM_TENSOR_BIT; }
  /**
  Set the arry type.
  */
  void ClearArrayType(const char *name){ this->Arrays[name]&=~ARRAY_TYPE_BITS; }
  void SetArrayType(const char *name, int type)
    {
    this->ClearArrayType(name);
    this->Arrays[name]|=type;
    }
  void SetArrayTypeToScalar(const char *name){ this->SetArrayType(name,VECTOR_BIT); }
  void SetArrayTypeToVector(const char *name){ this->SetArrayType(name,SCALAR_BIT); }
  void SetArrayTypeToTensor(const char *name){ this->SetArrayType(name,TENSOR_BIT); }
  void SetArrayTypeToSymetricTensor(const char *name){ this->SetArrayType(name,SYM_TENSOR_BIT); }
  /**
  Activate/Deactivate the named array so that it will/will not be read.
  */
  void ActivateArray(const char *name){ this->Arrays[name] |= ACTIVE_BIT; }
  void ActivateAllArrays();
  void DeactivateArray(const char *name){ this->Arrays[name] &= ~ACTIVE_BIT; }
  void DeactivateAllArrays();
  /// \@}


  /// \Section ArrayQuerries \@{
  /**
  Query the array type.
  */
  int IsArrayScalar(const char *name)
    {
    int status=this->Arrays[name];
    return status&SCALAR_BIT;
    }
  int IsArrayVector(const char *name)
    {
    int status=this->Arrays[name];
    return status&VECTOR_BIT;
    }
  int IsArrayTensor(const char *name)
    {
    int status=this->Arrays[name];
    return status&TENSOR_BIT;
    }
  int IsArraySymetricTensor(const char *name)
    {
    int status=this->Arrays[name];
    return status&SYM_TENSOR_BIT;
    }
  /**
  Query the named array's status
  */
  int IsArrayActive(const char *name)
    {
    int status=this->Arrays[name];
    return status&ACTIVE_BIT;
    }
  /**
  Return the number of scalars in the dataset.
  */
  size_t GetNumberOfArrays() const { return this->Arrays.size(); }

  /**
  Return the number of files comprising the dataset.
  */
  size_t GetNumberOfArrayFiles() const;

  /**
  Return the i-th array's name.
  */
  const char *GetArrayName(size_t i) const;
  /// \@}

  /// \Section TimeSupport \@{
  /**
  Return the requested time step id, if a metadat file has been
  successfully opened.
  */
  virtual void AddTimeStep(int t) { this->TimeSteps.push_back(t); }

  /**
  Return the number of time steps in the dataset, if a metadata
  file has been successfully opened.
  */
  size_t GetNumberOfTimeSteps() const { return this->TimeSteps.size(); }

  /**
  Clear the current timesteps
  */
  void ClearTimeSteps() { this->TimeSteps.clear(); }

  /**
  Return the requested time step id, if a metadata file has been
  successfully opened.
  */
  int GetTimeStep(size_t i) const { return this->TimeSteps[i]; }

  /**
  Return a pointer to the time steps array.
  */
  const int *GetTimeSteps() const { return &this->TimeSteps[0]; }
  /// \@}


  /**
  Implemantion's chance to add any specialized key,value pairs
  it needs into the pipeline information.
  */
  virtual void PushPipelineInformation(
        vtkInformation *,
        vtkInformation *)
  {}

  /**
  Serialize the object into a byte stream  Returns the
  size in bytes of the stream. Or 0 in case of an error.
  */
  virtual void Pack(BinaryStream &str);

  /**
  Initiaslize the object froma byte stream (see also Serialize)
  returns 0 in case of an error.
  */
  virtual void UnPack(BinaryStream &str);


  /// Print internal state.
  virtual void Print(std::ostream &os) const;

private:
  friend std::ostream &operator<<(std::ostream &os, const BOVMetaData &md);

protected:
  char Mode;                          // 'r' reading, 'w' writing
  int IsOpen;
  std::string FileName;               // path and file name of metadata file.
  std::string PathToBricks;           // path to the brick files.
  std::string BrickFileExtension;     // extension used for brick files
  CartesianExtent Domain;             // Dataset domain on disk.
  CartesianExtent Subset;             // Subset of interst to read.
  CartesianExtent Decomp;             // Part of the subset this process will read.
  std::map<std::string,int> Arrays;   // map of array names to a status flag.
  std::vector<int> TimeSteps;         // Time values.
  std::string DataSetType;            // vtk data set type string
  double Dt;                          // time step value
  double Origin[3];                   // dataset origin for image
  double Spacing[3];                  // grid spacing for image
  SharedArray<float> *Coordinates[3]; // x,y,z coordinate arrays for rectilinear
};

std::ostream &operator<<(std::ostream &os, const BOVMetaData &md);

#endif

// VTK-HeaderTest-Exclude: BOVMetaData.h
