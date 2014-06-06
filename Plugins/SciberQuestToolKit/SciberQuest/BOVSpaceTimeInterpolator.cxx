#include "vtkPVInformationKeys.h"


//-----------------------------------------------------------------------------
int BOVSpaceTimeInterpolator::Open(const char *file)
{
  this->Reader->SetCommunicator(MPI_COMM_WORLD);
  if(!this->Reader->Open(this->FileName))
    {
    vtkErrorMacro("Failed to open the file \"" << safeio(this->FileName) << "\".");
    return -1;
    }

  this->Reader->GetMetaData()->GetOrigin(this->X0);
  this->Reader->GetMetaData()->GetSpacing(this->Dx);


  // Determine which time steps are available.
  int nSteps=this->Reader->GetMetaData()->GetNumberOfTimeSteps();
  const int *steps=this->Reader->GetMetaData()->GetTimeSteps();
  std::vector<double> times(nSteps,0.0);
  for (int i=0; i<nSteps; ++i)
    {
    times[i]=(double)steps[i]; // use the index rather than the actual.
    }

  CartesianDecomp *ddecomp;

  // The file extents describe the data as it is on the disk.
  CartesianExtent fileExt=md->GetDomain();
  // shift to dual grid
  fileExt.NodeToCell();
  // we must always have a single cell in all directions.
  /*if ((fileExt[1]<fileExt[0])||(fileExt[3]<fileExt[2]))
    {
    vtkErrorMacro("Invalid fileExt requested: " << fileExt << ".");
    return 1;
    }*/
  // this is a hack to accomodate 2D grids.
  for (int q=0; q<3; ++q)
    {
    int qq=2*q;
    if (subset[qq+1]<subset[qq])
      {
      subset[qq+1]=subset[qq];
      }
    }

  // This reader can read vtkImageData, vtkRectilinearGrid, and vtkStructuredData

  if (this->Reader->DataSetTypeIsImage())
    {
    /// Image data

    // Pull origin and spacing we stored during RequestInformation pass.
    double dX[3];
    info->Get(vtkDataObject::SPACING(),dX);
    double X0[3];
    info->Get(vtkDataObject::ORIGIN(),X0);

    // dimensions of the dummy output
    int nPoints[3];
    nPoints[0]=this->WorldSize+1;
    nPoints[1]=2;
    nPoints[2]=2;

    // Configure the output.
    vtkImageData *idds=dynamic_cast<vtkImageData*>(output);
    idds->SetDimensions(nPoints);
    idds->SetOrigin(X0);
    idds->SetSpacing(dX);
    idds->SetExtent(decomp.GetData());

    // Store the bounds of the requested subset.
    double subsetBounds[6];
    subsetBounds[0]=X0[0];
    subsetBounds[1]=X0[0]+dX[0]*((double)this->WorldSize);
    subsetBounds[2]=X0[1];
    subsetBounds[3]=X0[1]+dX[1];
    subsetBounds[4]=X0[2];
    subsetBounds[5]=X0[2]+dX[2];
    info->Set(vtkPVInformationKeys::WHOLE_BOUNDING_BOX(),subsetBounds,6);
    req->Append(
        vtkExecutive::KEYS_TO_COPY(),
        vtkPVInformationKeys::WHOLE_BOUNDING_BOX());

    // Setup the user defined domain decomposition over the subset. This
    // decomposition is used to fine tune the I/O performance of out-of-core
    // filters.
    double *subsetX0=md->GetOrigin();
    double *subsetDX=md->GetSpacing();

    ImageDecomp *iddecomp=ImageDecomp::New();
    iddecomp->SetFileExtent(fileExt);
    iddecomp->SetExtent(subset);
    iddecomp->SetOrigin(subsetX0);
    iddecomp->SetSpacing(subsetDX);
    iddecomp->ComputeBounds();
    iddecomp->SetDecompDims(this->DecompDims);
    iddecomp->SetPeriodicBC(this->PeriodicBC);
    iddecomp->SetNumberOfGhostCells(this->NGhosts);
    int ok=iddecomp->DecomposeDomain();
    if (!ok)
      {
      vtkErrorMacro("Failed to decompose domain.");
      output->Initialize();
      return 1;
      }
    ddecomp=iddecomp;
    }
  else
  if (this->Reader->DataSetTypeIsRectilinear())
    {
    /// Rectilinear grid
    // Store the bounds of the requested subset.
    double subsetBounds[6]={
      md->GetCoordinate(0)->GetPointer()[subset[0]],
      md->GetCoordinate(0)->GetPointer()[subset[1]+1],
      md->GetCoordinate(1)->GetPointer()[subset[2]],
      md->GetCoordinate(1)->GetPointer()[subset[3]+1],
      md->GetCoordinate(2)->GetPointer()[subset[4]],
      md->GetCoordinate(2)->GetPointer()[subset[5]+1]};
    info->Set(vtkPVInformationKeys::WHOLE_BOUNDING_BOX(),subsetBounds,6);
    req->Append(
        vtkExecutive::KEYS_TO_COPY(),
        vtkPVInformationKeys::WHOLE_BOUNDING_BOX());

    // Store the bounds of the requested subset.
    int nCells[3];
    subset.Size(nCells);

    int nLocal=nCells[0]/this->WorldSize;
    int nLarge=nCells[0]%this->WorldSize;

    int ilo;
    int ihi;

    if (this->WorldRank<nLarge)
      {
      ilo=subset[0]+this->WorldRank*(nLocal+1);
      ihi=ilo+nLocal+1;
      }
    else
      {
      ilo=subset[0]+this->WorldRank*nLocal+nLarge;
      ihi=ilo+nLocal;
      }

    // Configure the output.
    vtkRectilinearGrid *rgds=dynamic_cast<vtkRectilinearGrid*>(output);

    vtkFloatArray *fa;
    float *pFa;
    fa=vtkFloatArray::New();
    fa->SetNumberOfTuples(2);
    pFa=fa->GetPointer(0);
    pFa[0]=md->GetCoordinate(0)->GetPointer()[ilo];
    pFa[1]=md->GetCoordinate(0)->GetPointer()[ihi];
    rgds->SetXCoordinates(fa);
    fa->Delete();

    fa=vtkFloatArray::New();
    fa->SetNumberOfTuples(2);
    pFa=fa->GetPointer(0);
    pFa[0]=subsetBounds[2];
    pFa[1]=subsetBounds[3];
    rgds->SetYCoordinates(fa);
    fa->Delete();

    fa=vtkFloatArray::New();
    fa->SetNumberOfTuples(2);
    pFa=fa->GetPointer(0);
    pFa[0]=subsetBounds[4];
    pFa[1]=subsetBounds[5];
    rgds->SetZCoordinates(fa);
    fa->Delete();

    rgds->SetExtent(decomp.GetData());

    // Setup the user defined domain decomposition over the subset. This
    // decomposition is used to fine tune the I/O performance of out-of-core
    // filters.
    RectilinearDecomp *rddecomp=RectilinearDecomp::New();
    rddecomp->SetFileExtent(fileExt);
    rddecomp->SetExtent(subset);
    rddecomp->SetDecompDims(this->DecompDims);
    rddecomp->SetPeriodicBC(this->PeriodicBC);
    rddecomp->SetNumberOfGhostCells(this->NGhosts);
    rddecomp->SetCoordinate(0,md->GetCoordinate(0));
    rddecomp->SetCoordinate(1,md->GetCoordinate(1));
    rddecomp->SetCoordinate(2,md->GetCoordinate(2));
    int ok=rddecomp->DecomposeDomain();
    if (!ok)
      {
      vtkErrorMacro("Failed to decompose domain.");
      output->Initialize();
      return 1;
      }
    ddecomp=rddecomp;
    }
  else
  if (this->Reader->DataSetTypeIsStructured())
    {
    /// Structured data
    vtkErrorMacro("vtkStructuredData is not implemented yet.");
    return 1;
    }
  else
    {
    /// unrecognized dataset type
    vtkErrorMacro(
      << "Error: invalid dataset type \""
      << md->GetDataSetType() << "\".");
    }

  // Pseduo read puts place holders for the selected arrays into
  // the output so they can be selected in downstream filters' GUIs.
  int ok;
  ok=this->Reader->ReadMetaTimeStep(stepId,output,this);
  if (!ok)
    {
    vtkErrorMacro(
      << "Read failed." << endl << *md);
    output->Initialize();
    return 1;
    }

  // Put a reader into the pipeline, downstream filters can
  // the read on demand.
  vtkSQOOCBOVReader *OOCReader=vtkSQOOCBOVReader::New();
  OOCReader->SetReader(this->Reader);
  OOCReader->SetTimeIndex(stepId);
  OOCReader->SetDomainDecomp(ddecomp);
  OOCReader->SetBlockCacheSize(this->BlockCacheSize);
  OOCReader->SetCloseClearsCachedBlocks(this->ClearCachedBlocks);
  OOCReader->InitializeBlockCache();
  info->Set(vtkSQOOCReader::READER(),OOCReader);
  OOCReader->Delete();
  ddecomp->Delete();
  req->Append(vtkExecutive::KEYS_TO_COPY(),vtkSQOOCReader::READER());

  return 0;
}

//-----------------------------------------------------------------------------
void BOVSpaceTimeInterpolator::Close();


//-----------------------------------------------------------------------------
void BOVSpaceTimeInterpolator::SetSubset(const int *s)
{
  this->SetSubset(s[0],s[1],s[2],s[3],s[4],s[5]);
}

//-----------------------------------------------------------------------------
void BOVSpaceTimeInterpolator::SetSubset(
      int ilo,
      int ihi,
      int jlo,
      int jhi,
      int klo,
      int khi)
{
  CartesianExtent subset(ilo,ihi,jlo,jhi,klo,khi);
  this->Reader->GetMetaData()->SetSubset(subset);
}

//-----------------------------------------------------------------------------
int BOVSpaceTimeInterpolator::GetNumberOfTimeSteps()
{
  return this->Reader->GetMetaData()->GetNumberOfTimeSteps();
}

//-----------------------------------------------------------------------------
void BOVSpaceTimeInterpolator::GetTimeSteps(std::vector<double> &times)
{
  int n=this->Reader->GetMetaData()->GetNumberOfTimeSteps();
  times.resize(n);
  for (int i=0; i<n; ++i)
    {
    times[i]=this->Reader->GetMetaData()->GetTimeStep(i);
    }
}

//-----------------------------------------------------------------------------
void BOVSpaceTimeInterpolator::SetArrayStatus(const char *name, int status)
{
  if (status)
    {
    this->Reader->GetMetaData()->ActivateArray(name);
    }
  else
    {
    this->Reader->GetMetaData()->DeactivateArray(name);
    }
}

//-----------------------------------------------------------------------------
int BOVSpaceTimeInterpolator::GetArrayStatus(const char *name)
{
  return this->Reader->GetMetaData()->IsArrayActive(name);
}

//-----------------------------------------------------------------------------
int BOVSpaceTimeInterpolator::GetNumberOfArrays()
{
  return this->Reader->GetMetaData()->GetNumberOfArrays();
}

//-----------------------------------------------------------------------------
const char* BOVSpaceTimeInterpolator::GetArrayName(int idx)
{
  return this->Reader->GetMetaData()->GetArrayName(idx);
}

//-----------------------------------------------------------------------------
void BOVSpaceTimeInterpolator::ClearArrayStatus()
{
  int nArrays=this->GetNumberOfArrays();
  for (int i=0; i<nArrays; ++i)
    {
    const char *aName=this->GetArrayName(i);
    this->SetArrayStatus(aName,0);
    }
}

//-----------------------------------------------------------------------------
int BOVSpaceTimeInterpolator::SetTime(double t)
{
  // short circuit if we already have the requisite timesteps cached.
  if ((t>=this->Cache[0]->GetTime())&&(t<=this->Cache[1]->GetTime()))
    {
    return 0;
    }

  // requested time is not in the cache
  // close/reclaim
  this->Cache[0]->Close();
  this->Cache[1]->Close();

  // intialize the cache with bracketing timesteps
  int nSteps=this->Times.size();
  for (int i=0; i<nSteps; ++i)
    {
    if (t<this->Times[i])
      {
      int stepId=this->Reader->GetMetaData()->GetTimeStep(i);

      this->T0=this->Times[i];
      double T1=this->Times[i+1];
      this->Dt=T1-this->T0;

      this->Cache[0]->SetTimeIndex(stepId);
      this->Cache[0]->SetTime(this->T0);
      this->Cache[0]->Open();

      this->Cache[1]->SetTimeIndex(stepId+1);
      this->Cache[1]->SetTime(T1);
      this->Cache[1]->Open();

      return 0;
      }
    }

  // t is out of bounds
  return -1;
}

//-----------------------------------------------------------------------------
int UpdateCache(double *x, double t)
{
  // initialize the cache with bracketing time steps
  int iErr;
  iErr=this->SetTime(t);
  if (iErr)
    {
    // requested t is out of bounds
    return -1;
    }

  // read data if we don't currenty have it
  if (!this->WorkingDomain.Contains(x))
    {
    this->Data[0]=dynamic_cast<vtkDataSet*>(
          this->Cache[0]->ReadNeighborhood(x,this->WorkingExtent));

    this->Data[0]->GetOrigin(this->X0);
    this->Data[0]->GetSpacing(this->Dx);

    this->WorkingExtent.GetBounds(
          this->X0,this->Dx,this->WorkingDomain.GetData());

    this->ActiveArray[0]
      = this->Data[0]->GetPointData()->GetArray(this->ArrayName.c_str()):

    this->Data[1]=dynamic_cast<vtkDataSet*>(
          this->Cache[1]->ReadNeighborhood(x,this->WorkingExtent));

    this->ActiveArray[1]
      = this->Data[1]->GetPointData()->GetArray(this->ArrayName.c_str()):

    }

  return 0;
}

//-----------------------------------------------------------------------------
void BOVSpaceTimeInterpolator::IndexOf(double *X, int *I)
{
  I[0]=(X[0]-this->X0[0])/this->Dx[0];
  I[1]=(X[1]-this->X0[1])/this->Dx[1];
  I[2]=(X[2]-this->X0[2])/this->Dx[2];
}

//-----------------------------------------------------------------------------
void BOVSpaceTimeInterpolator::ParametricCoordinates(
      double t,
      double &tau)
{
  tau=(t-this->T0)/this->Dt;
}

//-----------------------------------------------------------------------------
void BOVSpaceTimeInterpolator::ParametricCoordinates(
      int *I,
      double *X,
      double *Xi)
{
  Xi[0]=X[0]-I[0]*this->Dx[0];
  Xi[1]=X[1]-I[1]*this->Dx[1];
  Xi[2]=X[2]-I[2]*this->Dx[2];
}

//-----------------------------------------------------------------------------
template<typename T>
int BOVSpaceTimeInterpolator::Interpolate(
      int *I,
      double *Xi,
      double tau,
      int nComp,
      T *V0,
      T *V1,
      T *W)
{
  switch (this->DimMode)
    {
    case CartesianExtent::DIM_MODE_2D_XY:
    case CartesianExtent::DIM_MODE_2D_XZ:
    case CartesianExtent::DIM_MODE_2D_YZ:
      {
      int vi=I[this->SlowDim]*this->Ni+I[this->FastDim];
      int vi0=vi*nComp;
      int vi1=vi0+nComp;
      int vi2=vi0+this->Ni*nComp;
      int vi3=vi2+nComp;
      for (int c=0; c<nComp; ++c)
        {
        T w0=Interpolate(
              Xi[this->FastDim],
              Xi[this->SlowDim],
              V0[vi0+c],
              V0[vi1+c],
              V0[vi2+c],
              V0[vi3+c]);

        T w1=Interpolate(
              Xi[this->FastDim],
              Xi[this->SlowDim],
              V1[vi0+c],
              V1[vi1+c],
              V1[vi2+c],
              V1[vi3+c]);

        W[c]=Interpolate(tau,w0,w1);
        }
      }
      break;
    case CartesianExtent::DIM_MODE_3D:
      {
      int vi=I[2]*this->Nij+I[1]*this->Ni+I[0];
      int vi0=vi*nComp;
      int vi1=vi0+nComp;
      int vi2=vi0+this->Ni*nComp;
      int vi3=vi2+nComp;

      int vi4=vi0+nComp*this->Nij;
      int vi5=vi4+nComp;
      int vi6=vi4+this->Ni*nComp;
      int vi7=vi6+nComp;

      for (int c=0; c<nComp; ++c)
        {
        T w0=Interpolate(
              Xi[0],
              Xi[1],
              Xi[2],
              V0[vi0+c],
              V0[vi1+c],
              V0[vi2+c],
              V0[vi3+c],
              V0[vi4+c],
              V0[vi5+c],
              V0[vi6+c],
              V0[vi7+c]);

        T w1=Interpolate(
              Xi[0],
              Xi[1],
              Xi[2],
              V1[vi0+c],
              V1[vi1+c],
              V1[vi2+c],
              V1[vi3+c],
              V1[vi4+c],
              V1[vi5+c],
              V1[vi6+c],
              V1[vi7+c]);

        W[c]=Interpolate(tau,w0,w1);
        }
      }
      break;
    default:
      dqErrorMacro(pCerr(),"Invalid mode " << this->Mode);
      return -1;
    }
  return 0;
}

//-----------------------------------------------------------------------------
template<typename T>
int BOVSpaceTimeInterpolator::Interpolate(
      int *I,
      double *Xi,
      double tau
      vtkDataSet *output)
{
  int nArrays=this->Data[0]->GetPointData()->GetNumberOfArrays();

  switch (this->DimMode)
    {
    case CartesianExtent::DIM_MODE_2D_XY:
    case CartesianExtent::DIM_MODE_2D_XZ:
    case CartesianExtent::DIM_MODE_2D_YZ:
      {
      int vi=I[this->SlowDim]*this->Ni+I[this->FastDim];
      int vi0=vi;
      int vi1=vi0+1;
      int vi2=vi0+this->Ni;
      int vi3=vi2+1;

      for (int q=0; q<nArrays; ++q)
        {
        vtkDataArray *daV0=this->Data[0]->GetPointData()->GetArray(q);
        vtkDataArray *daV1=this->Data[1]->GetPointData()->GetArray(q);
        vtkDataArray *daW=output->GetPointData()->GetArray(q);

        int nComp=V0->GetNumberOfComponents();
        int nW=daW->GetNumberOfTuples();
        nW*=nComp;

        switch(inArray->GetDataType())
          {
          vtkFloatTemplateMacro(
            {
            VTK_TT *V0=(VTK_TT*)daV0->GetVoidPointer(0);
            VTK_TT *V1=(VTK_TT*)daV1->GetVoidPointer(0);
            VTK_TT *W=(VTK_TT*)daW->WriteVoidPointer(nW,nW+nComp);

            for (int c=0; c<nComp; ++c)
              {
              int _vi0=nComp*vi0+c;
              int _vi1=nComp*vi1+c;
              int _vi2=nComp*vi2+c;
              int _vi3=nComp*vi3+c;

              T w0=Interpolate(
                    Xi[this->FastDim],
                    Xi[this->SlowDim],
                    V0[_vi0],
                    V0[_vi1],
                    V0[_vi2],
                    V0[_vi3]);

              T w1=Interpolate(
                    Xi[this->FastDim],
                    Xi[this->SlowDim],
                    V1[_vi0],
                    V1[_vi1],
                    V1[_vi2],
                    V1[_vi3]);

              W[c]=Interpolate(tau,w0,w1);
              }
            }
            );

          default:
            sqErrorMacro(std::cerr,"Unsupprted numeric type " << inArray->GetClassName()):
          }
        }
      }
      break;

    case CartesianExtent::DIM_MODE_3D:
      {
      int vi=I[2]*this->Nij+I[1]*this->Ni+I[0];
      int vi0=vi;
      int vi1=vi0+1;
      int vi2=vi0+this->Ni;
      int vi3=vi2+1;

      int vi4=vi0+this->Nij;
      int vi5=vi4+1;
      int vi6=vi4+this->Ni;
      int vi7=vi6+1;

      for (int q=0; q<nArrays; ++q)
        {
        vtkDataArray *daV0=this->Data[0]->GetPointData()->GetArray(q);
        vtkDataArray *daV1=this->Data[1]->GetPointData()->GetArray(q);
        vtkDataArray *daW=output->GetPointData()->GetArray(q);

        int nComp=V0->GetNumberOfComponents();
        int nW=daW->GetNumberOfTuples();
        nW*=nComp;

        switch(inArray->GetDataType())
          {
          vtkFloatTemplateMacro(
            {
            VTK_TT *V0=(VTK_TT*)daV0->GetVoidPointer(0);
            VTK_TT *V1=(VTK_TT*)daV1->GetVoidPointer(0);
            VTK_TT *W=(VTK_TT*)daW->WriteVoidPointer(nW,nW+nComp);

            for (int c=0; c<nComp; ++c)
              {
              int _vi0=nComp*vi0+c;
              int _vi1=nComp*vi1+c;
              int _vi2=nComp*vi2+c;
              int _vi3=nComp*vi3+c;

              int _vi4=nComp*vi4+c;
              int _vi5=nComp*vi5+c;
              int _vi6=nComp*vi6+c;
              int _vi7=nComp*vi7+c;

              T w0=Interpolate(
                    Xi[0],
                    Xi[1],
                    Xi[2],
                    V0[vi0+c],
                    V0[vi1+c],
                    V0[vi2+c],
                    V0[vi3+c],
                    V0[vi4+c],
                    V0[vi5+c],
                    V0[vi6+c],
                    V0[vi7+c]);

              T w1=Interpolate(
                    Xi[0],
                    Xi[1],
                    Xi[2],
                    V1[vi0+c],
                    V1[vi1+c],
                    V1[vi2+c],
                    V1[vi3+c],
                    V1[vi4+c],
                    V1[vi5+c],
                    V1[vi6+c],
                    V1[vi7+c]);

              W[c]=Interpolate(tau,w0,w1);
              }
            }
            );

          default:
            sqErrorMacro(std::cerr,"Unsupprted numeric type " << inArray->GetClassName()):
          }
        }
      }
      break;
    default:
      dqErrorMacro(pCerr(),"Invalid mode " << this->Mode);
      return -1;
    }
  return 0;
}
