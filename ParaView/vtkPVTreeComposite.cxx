/*=========================================================================
  
  Program:   Visualization Toolkit
  Module:    vtkPVTreeComposite.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  
Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THxEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkPVTreeComposite.h"
#include "vtkObjectFactory.h"


//-------------------------------------------------------------------------
vtkPVTreeComposite* vtkPVTreeComposite::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVTreeCompssite");
  if(ret)
    {
    return (vtkPVTreeComposite*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPVTreeComposite;
}


//#########################################################################
// If we are not using MPI, just stub out this class so the supper class
// will do every thing.
#ifdef VTK_USE_MPI

//-------------------------------------------------------------------------
vtkPVTreeComposite::vtkPVTreeComposite()
{
  this->MPIController = vtkMPIController::SafeDownCast(this->Controller);
  
  this->RootWaiting = 0;
  this->ReceivePending = 0;
  this->ReceiveMessage = 0;
  
  if (this->MPIController == NULL)
    {
    vtkErrorMacro("This objects requires an MPI controller.");
    }
  this->LocalProcessId = this->Controller->GetLocalProcessId();
  this->RenderAborted = 0;
  
  this->RenderView = NULL;
}

  
//-------------------------------------------------------------------------
vtkPVTreeComposite::~vtkPVTreeComposite()
{
  this->MPIController = NULL;
  
  this->SetRenderView(NULL);
  
  // sanity check
  if (this->ReceivePending)
    {
    vtkErrorMacro("A receive is still pending.");
    }  
    
}


//----------------------------------------------------------------------------
void vtkPVTreeComposite::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkTreeComposite::PrintSelf(os, indent);
}


//----------------------------------------------------------------------------
void vtkPVTreeComposite::CheckForAbortRender()
{
  int abort;
  
  if (this->RenderAborted)
    {
    return;
    }
  
  if (this->LocalProcessId == 0)
    {
    abort = this->RootAbortCheck();
    }
  else
    {
    abort = this->SatelliteAbortCheck();
    }
  
  if (abort)
    {
    this->RenderAborted = 1;
    }
}

//----------------------------------------------------------------------------
int vtkPVTreeComposite::CheckForAbortComposite()
{
  int abort;
  
  if (this->LocalProcessId == 0)
    {
    abort = this->RootFinalAbortCheck();
    }
  else
    {
    abort = this->SatelliteFinalAbortCheck();
    }
  
  // Reset this for the next render
  this->RenderAborted = 0;
  return abort;
}




#define STATUS_TAG  548934



// Messages sent from satellite processes to process 0.
// Used to ping the root process.
#define  VTK_CHECK_ABORT       0
// Tells the root that rendering is finished.
#define  VTK_FINISHED          1

// Messages sent from the root to the satellite processes.
// Abort rendering as soon as possible.
#define  VTK_ABORT_RENDER      2
// Root is waiting for you to finish and expects to be pinged.
#define  VTK_ROOT_WAITING      3
// Abort the composite message.  RootWaiting will always come before this message.
#define  VTK_ABORT_COMPOSITE   4
// Go ahead and composite.  RootWaiting will always come before this message.
#define  VTK_COMPOSITE         5



//------------- Methods for Satellite Processes --------------


// Out process is finished rendering now and will wait for the final status message from root.
int vtkPVTreeComposite::SatelliteFinalAbortCheck()
{
  int status = VTK_FINISHED;

  //cout << this->LocalProcessId << ": SatelliteFinalAbortCheck\n";

  // Get rid of any un resolved receives.
  // Only AbortRender and RootWaiting messages can be received before the "Finished" message.
  // We can ignore both.
  if (this->ReceivePending)
    {
    //cout << "1: Cancel last qued receive.\n";
    MPI_Cancel(&this->ReceiveRequest.Req);
    this->ReceivePending = 0;
    }  
  
  // We might want to wait to send this until after the root says it is waiting for us to finish.
  //cout << "1 send to 0, message: " << status << endl;
  this->MPIController->Send(&status, 1, 0, STATUS_TAG);
  
  // Wait for a confirmation to continue or abort.
  while (1)
    {
    //cout << "1: Entering blocking receive\n";
    this->MPIController->Receive(&status, 1, 0, STATUS_TAG);

    // Ignore AbortRender message:  Rendering is complete.
    // Ignore RootWaiting message:  We have already sent a "Finished" message to root.
  
    //cout << "1: Received from 0, message: " << status << endl;
    
    if (status == VTK_COMPOSITE)
      {
      //cout << this->LocalProcessId << ": SatelliteFinalAbortCheck: Returning: 0\n";
      // Reset the rootWaiting folag for the next render.
      this->RootWaiting = 0;
      return 0;
      }
    else if (status == VTK_ABORT_COMPOSITE)
      {
      //cout << this->LocalProcessId << ": SatelliteFinalAbortCheck: Returning: 1\n";
      // Reset the rootWaiting flag for the next render.
      this->RootWaiting = 0;
      return 1;
      }
    else
      {
      //cout << "1: ignore message: " << status << endl;
      }
    //cout << "1: end while\n";
    }
}


int vtkPVTreeComposite::SatelliteAbortCheck()
{
  int status;

  //cout << this->LocalProcessId << ": SatelliteAbortCheck\n";
  
  // If the root is waiting on us, then ping it so that it can check for an abort.
  if (this->RootWaiting)
    {
    //cout << "1: Ping root\n";
    vtkMPICommunicator::Request sendRequest;
    status = VTK_CHECK_ABORT;
    //cout << "1 noBlockSend to 0, message: " << status << endl;
    this->MPIController->NoBlockSend(&status, 1, 0, STATUS_TAG, sendRequest);
    }
  
  // If this is the first call for this render, 
  // then we need to setup the receive message.
  if ( ! this->ReceivePending)
    {
    //cout << "1: Que no Block Receive from 0\n";
    this->MPIController->NoBlockReceive(&this->ReceiveMessage, 1, 0, STATUS_TAG, 
				       this->ReceiveRequest);
    this->ReceivePending = 1;
    }
  
  if (this->ReceivePending && this->ReceiveRequest.Test())
    { // Received a message from the root.
    this->ReceivePending = 0;
    //cout << "1 NoBlockReceived from 0, message: " << this->ReceiveMessage << endl;
    if (this->ReceiveMessage == VTK_ABORT_RENDER)
      {  // Root is telling us to short circuit the render.
      //cout << this->LocalProcessId << ": Statelite received an abort message.\n";
      // .... set abort flag of render window.....
      this->RenderWindow->SetAbortRender(1);
      // Rearm the receive to get another message.
      //cout << "1: Que no Block Receive from 0\n";
      this->MPIController->NoBlockReceive(&this->ReceiveMessage, 1, 0, STATUS_TAG, 
					  this->ReceiveRequest);
      this->ReceivePending = 1;
      //cout << this->LocalProcessId << ": SatelliteAbortCheck Returning: 1\n";
      return 1;
      }
    
    if (this->ReceiveMessage == VTK_ROOT_WAITING)
      { // Root is finished rendering, and is waiting for this process.  It wants to be pinged.
      //cout << this->LocalProcessId << ": Statelite received a RootWaiting message.\n";
      this->RootWaiting = 1;
      // Rearm the receive to get a possible abort.
      //cout << "1: Que no Block Receive from 0\n";
      this->MPIController->NoBlockReceive(&this->ReceiveMessage, 1, 0, STATUS_TAG, this->ReceiveRequest);
      this->ReceivePending = 1;
      }
    }
  
  //cout << this->LocalProcessId << ": SatelliteAbortCheck Returning: 0\n";
  return 0;
}








//------------- Methods for Root Processes --------------

// Count is temporary for testing.
int vtkPVTreeComposite::RootAbortCheck()
{
  //sleep(5);
  
  //cout << this->LocalProcessId << ": RootAbortCheck\n";

  // Never abort while printing.
  if (this->RenderView->GetPrinting())
    {
    //cout << this->LocalProcessId << ": RootAbortCheck Return 0 (print)\n";
    return 0;
    }
  
  // This checks for events to decide whether to abort.
  if (this->RenderView->ShouldIAbort())
    { // Yes, abort.
    int idx;
    int status = VTK_ABORT_RENDER;
    int num = this->MPIController->GetNumberOfProcesses();
    // Tell the satellite processes they need to abort.

    //cout << "Root  ---------- ABORT ----------- \n";

    for (idx = 1; idx < num; ++idx)
      {
      //cout << "0 send to 1, message: " << status << endl;
      this->MPIController->Send(&status, 1, idx, STATUS_TAG);
      }
    // abort our own render.
    this->RenderWindow->SetAbortRender(1);

    //cout << this->LocalProcessId << ": RootAbortCheck Return 1\n";
    return 1;
    }

  //cout << this->LocalProcessId << ": RootAbortCheck Return 0\n";
  return 0;
}


// "abort" is true if rendering was previously aborted.
int vtkPVTreeComposite::RootFinalAbortCheck()
{
  int waitingFlag;
  int idx;
  int num;
  int status;
  int abort = this->RenderAborted;

  //sleep(2);

  //cout << this->LocalProcessId << ": RootFinalAbortCheck\n";
  
  // Wait for all the satelite processes to finish.
  num = this->MPIController->GetNumberOfProcesses();
  for (idx = 1; idx < num; ++idx)
    {
    // Send a message to the next process that informs it that we are waiting for it to render.
    status = VTK_ROOT_WAITING;
    //cout << "0 send to 1, message: " << status << endl;
    this->MPIController->Send(&status, 1, idx, STATUS_TAG);
    // Wait for the process to finish.
    waitingFlag = 1;
    while ( waitingFlag)
      {
      this->MPIController->Receive(&status, 1, idx, STATUS_TAG);
      //cout << "0 Received from 1: message: " << status << endl;
      if (status == VTK_FINISHED)
	{
	waitingFlag = 0;
	}
      else if (status != VTK_CHECK_ABORT)
	{
	//cout << "Sanity check failed: Expective CheckAbort or Finished message.\n";
	}
      else if ( ! abort)
	{
	//cout << "Root pinged !!!\n";
	// Keep checking for aborts.  
	// It is OK to send an abort message to processes after they are finished.
	abort = this->RootAbortCheck();
	}
      }
    }
  
  // Now send the final message to all of the satelite processes.
  // It will cause them to composite or skip the compositing step.
  if (abort)
    {
    //cout << "Root  ---------- ABORT Composite----------- \n";
    status = VTK_ABORT_COMPOSITE;
    }
  else
    {
    status = VTK_COMPOSITE;
    }
  for (idx = 1; idx < num; ++idx)
    {
    //cout << "0 send to 1, message: " << status << endl;
    this->MPIController->Send(&status, 1, idx, STATUS_TAG);
    }
  
  //cout << this->LocalProcessId << ": RootFinalAbortCheck, Returning: " << abort << "\n";
  return abort;
}


// end VTK_USE_MPI
//#########################################################################
#else
// Not using MPI

//-------------------------------------------------------------------------
vtkPVTreeComposite::vtkPVTreeComposite()
{ 
  this->RenderView = NULL;
}

  
//-------------------------------------------------------------------------
vtkPVTreeComposite::~vtkPVTreeComposite()
{
  this->SetRenderView(NULL);
}


//----------------------------------------------------------------------------
void vtkPVTreeComposite::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkTreeComposite::PrintSelf(os, indent);
}


//----------------------------------------------------------------------------
void vtkPVTreeComposite::CheckForAbortRender()
{
  this->vtkTreeComposite::CheckForAbortRender();
}


//----------------------------------------------------------------------------
int vtkPVTreeComposite::CheckForAbortComposite()
{
  return this->vtkTreeComposite::CheckForAbortComposite();
}





#endif  // VTK_USE_MPI





