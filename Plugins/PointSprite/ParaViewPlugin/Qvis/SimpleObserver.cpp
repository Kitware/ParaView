/*****************************************************************************
*
* Copyright (c) 2000 - 2007, The Regents of the University of California
* Produced at the Lawrence Livermore National Laboratory
* All rights reserved.
*
* This file is part of VisIt. For details, see http://www.llnl.gov/visit/. The
* full copyright notice is contained in the file COPYRIGHT located at the root
* of the VisIt distribution or at http://www.llnl.gov/visit/copyright.html.
*
* Redistribution  and  use  in  source  and  binary  forms,  with  or  without
* modification, are permitted provided that the following conditions are met:
*
*  - Redistributions of  source code must  retain the above  copyright notice,
*    this list of conditions and the disclaimer below.
*  - Redistributions in binary form must reproduce the above copyright notice,
*    this  list of  conditions  and  the  disclaimer (as noted below)  in  the
*    documentation and/or materials provided with the distribution.
*  - Neither the name of the UC/LLNL nor  the names of its contributors may be
*    used to  endorse or  promote products derived from  this software without
*    specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT  HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR  IMPLIED WARRANTIES, INCLUDING,  BUT NOT  LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND  FITNESS FOR A PARTICULAR  PURPOSE
* ARE  DISCLAIMED.  IN  NO  EVENT  SHALL  THE  REGENTS  OF  THE  UNIVERSITY OF
* CALIFORNIA, THE U.S.  DEPARTMENT  OF  ENERGY OR CONTRIBUTORS BE  LIABLE  FOR
* ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT  LIMITED TO, PROCUREMENT OF  SUBSTITUTE GOODS OR
* SERVICES; LOSS OF  USE, DATA, OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER
* CAUSED  AND  ON  ANY  THEORY  OF  LIABILITY,  WHETHER  IN  CONTRACT,  STRICT
* LIABILITY, OR TORT  (INCLUDING NEGLIGENCE OR OTHERWISE)  ARISING IN ANY  WAY
* OUT OF THE  USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
* DAMAGE.
*
*****************************************************************************/

#include <SimpleObserver.h>

// *******************************************************************
// Method: SimpleObserver::SimpleObserver
//
// Purpose: 
//   Constructor for the Observer class.
//
// Programmer: Brad Whitlock
// Creation:   Thu Jun 1 11:31:06 PDT 2000
//
// Modifications:
//   
// *******************************************************************

SimpleObserver::SimpleObserver()
{
    doUpdate = true;
}

// *******************************************************************
// Method: SimpleObserver::~SimpleObserver
//
// Purpose: 
//   Destructor for the Observer class.
//
// Programmer: Brad Whitlock
// Creation:   Thu Jun 1 11:31:39 PDT 2000
//
// Modifications:
//   
// *******************************************************************

SimpleObserver::~SimpleObserver()
{
    // nothing special here.
}

// *******************************************************************
// Method: SimpleObserver::SetUpdate
//
// Purpose: 
//   Sets a flag that indicates whether or not the Observer's Update
//   method should be called by the Observer's subject.
//
// Arguments:
//    update : Tells the subject we're observing if the observer's
//             Update method needs to be called during a Notify.
//
// Returns:    
//
// Note:       
//
// Programmer: Brad Whitlock
// Creation:   Thu Jun 1 11:32:17 PDT 2000
//
// Modifications:
//   
// *******************************************************************

void
SimpleObserver::SetUpdate(bool update)
{
    doUpdate = update;
}

// *******************************************************************
// Method: SimpleObserver::GetUpdate
//
// Purpose: 
//   Gets the value of the update flag.
//
// Arguments:
//
// Returns:    The value of the update flag.
//
// Note:       
//
// Programmer: Brad Whitlock
// Creation:   Thu Jun 1 11:32:23 PDT 2000
//
// Modifications:
//   
// *******************************************************************

bool
SimpleObserver::GetUpdate()
{
    return doUpdate;
}

// *******************************************************************
// Method: SimpleObserver::SubjectRemoved
//
// Purpose: 
//   Tells the observer that it should not try and detach from the
//   subject.
//
// Programmer: Brad Whitlock
// Creation:   Thu Aug 31 15:19:49 PST 2000
//
// Modifications:
//   
// *******************************************************************

void
SimpleObserver::SubjectRemoved(Subject *)
{
    // nothing
}
