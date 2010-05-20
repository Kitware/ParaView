#include "vtkPriorityHelper.h"
#include "vtkObjectFactory.h"

#include "vtkAlgorithm.h"
#include "vtkAlgorithmOutput.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkPriorityHelper);

#define DEBUGPRINT_PRIORITY(arg)\
  if (this->EnableStreamMessages)                       \
    {                                                   \
    arg;                                                \
    }

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
                                            int vtkNotUsed(ghostLevel),
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
      DEBUGPRINT_PRIORITY(
        cerr << "PHelper(" << this << ") SetSplitUE " 
             << piece*numPasses+offset << "/" << numPieces*numPasses << endl;
                          );
      int ret = sddp->SetSplitUpdateExtent(port, piece*numPasses, 
                                           offset, 
                                           numPieces*numPasses, 0);      
      return ret;
      }
    }
  return 0;
}

//-----------------------------------------------------------------------------
vtkDataObject *vtkPriorityHelper::InternalUpdate(bool ReturnObject)
{
  //run through available pieces
  //look for first one (if any) with nonzero priority and get that
  if (this->Input)
    {
    double ret = 0.0;
    int i = 0;
    while (ret == 0.0 && i < this->NumPasses)
      {
      ret = this->ComputePriority();
      DEBUGPRINT_PRIORITY(
                          cerr << "PHelper(" << this << ") Priority on " 
                          << (this->Piece*this->NumPasses + i) << " was " << ret << endl;
                          );
      i++;
      if (ret == 0.0) //not useful, move along to next one
        {
        DEBUGPRINT_PRIORITY(
                            cerr << "PHelper(" << this << ") Skipping " 
                            << (this->Piece*this->NumPasses + (i-1)) << endl;
                            );
        this->SetSplitUpdateExtent(this->Port, this->Piece, i, 
                                   this->NumPieces, this->NumPasses, 0, 0);
        }
      }
    if (ret > 0.0)
      {
      if (ReturnObject)
        {       
        return this->Input->GetOutputDataObject(this->Port);
        }
      else
        {
        this->Input->Update();      
        return NULL;
        }
      }
    else
      {
      this->SetSplitUpdateExtent(this->Port, this->Piece, 0, 
                                 this->NumPieces, this->NumPasses, 0, 0);
      DEBUGPRINT_PRIORITY(
                          cerr << "PHelper(" << this << ") Nothing worth updating for." << endl;
                          );
      }
    }
  return NULL;
}

//-----------------------------------------------------------------------------
vtkDataObject *vtkPriorityHelper::ConditionallyGetDataObject()
{
  return this->InternalUpdate(true);
}

//-----------------------------------------------------------------------------
void vtkPriorityHelper::ConditionallyUpdate()
{
  this->InternalUpdate(false);
}

