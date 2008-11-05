#include "vtkPriorityHelper.h"
#include "vtkObjectFactory.h"

#include "vtkAlgorithm.h"
#include "vtkAlgorithmOutput.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkCxxRevisionMacro(vtkPriorityHelper, "1.1");
vtkStandardNewMacro(vtkPriorityHelper);

//-----------------------------------------------------------------------------
vtkPriorityHelper::vtkPriorityHelper()
{
  this->Input = NULL;
  this->Port = 0;
  this->Piece = 0;
  this->Offset = 0;
  this->NumPieces = 0;
  this->NumPasses = 0;
  this->EnableCulling = 1;
  this->EnableStreamMessages = 0;
}

//-----------------------------------------------------------------------------
vtkPriorityHelper::~vtkPriorityHelper()
{
}

//-----------------------------------------------------------------------------
double vtkPriorityHelper::ComputePriority()
{ 
  //find priority for currently active piece
  if (this->Input && this->EnableCulling)
    {
    double ret = this->Input->ComputePriority();
    return ret;
    }
  return 1.0;
}

//-----------------------------------------------------------------------------
void vtkPriorityHelper::SetInputConnection(vtkAlgorithmOutput *port)
{
  this->Input = NULL;
  if (port && port->GetProducer())
    {
    this->Input = port->GetProducer();
    }
}

//-----------------------------------------------------------------------------
int vtkPriorityHelper::SetSplitUpdateExtent(int port, 
                                            int piece, int offset,
                                            int numPieces, 
                                            int numPasses,
  int ghostLevel,
  int save)
{
  //set currently active piece, remember settings to reuse internally
  if (this->Input)
    {
    vtkStreamingDemandDrivenPipeline *sddp = 
      vtkStreamingDemandDrivenPipeline::SafeDownCast(
        this->Input->GetExecutive());
    if (sddp)
      {
      if (save)
        {
        this->Port = port;
        this->Piece = piece;
        this->Offset = offset;
        this->NumPieces = numPieces;
        this->NumPasses = numPasses;
        }
      int ret = sddp->SetSplitUpdateExtent(port, piece, 
                                           offset, 
                                           numPieces*numPasses, 0);      
      return ret;
      }
    }
  return 0;
}

//-----------------------------------------------------------------------------
vtkDataObject *vtkPriorityHelper::ConditionallyGetDataObject()
{
  //run through available pieces
  //look for first one (if any) with nonzero priority and get that
  if (this->Input)
    {
    double ret = 0.0;
    for (int i = 0; i < this->NumPasses;)
      {
      ret = this->ComputePriority();
      if (ret > 0.0)
        {
        break;
        } 
      if (this->EnableStreamMessages)
        {
        cerr << "PHelper(" << this << ") Skipping GetDataObject on " 
             << (this->Piece + i) << endl;
        }
      i++;
      this->SetSplitUpdateExtent(this->Port, this->Piece, i, 
                                 this->NumPieces, this->NumPasses, 0, 0);
      }
    if (ret > 0.0)
      {
      return this->Input->GetOutputDataObject(this->Port);
      }
    else
      {
      if (this->EnableStreamMessages)
        {
        cerr << "PHelper(" << this << ") No DataObject to get." << endl;
        }
      }
    }
  return NULL;
}

//-----------------------------------------------------------------------------
void vtkPriorityHelper::ConditionallyUpdate()
{
  //run through available pieces
  //look for first one (if any) with nonzero priority and update that
  if (this->Input)
    {
    double ret = 0.0;
    for (int i = 0; i < this->NumPasses;)
      {
      ret = this->ComputePriority();
      if (ret > 0.0)
        {
        break;
        }
      if (this->EnableStreamMessages)
        {
        cerr << "PHelper(" << this << ") Skipping Update on " 
             << (this->Piece + i) << endl;
        }
      i++;
      this->SetSplitUpdateExtent(this->Port, this->Piece, i, 
                                 this->NumPieces, this->NumPasses, 0, 0);
      }
    if (ret > 0.0)
      {
      this->Input->Update();      
      }
    else
      {
      if (this->EnableStreamMessages)
        {
        cerr << "PHelper(" << this << ") Not worth updating. " << endl;
        }
      }
    }
}

