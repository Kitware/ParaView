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

#include <Subject.h>
#include <SimpleObserver.h>

// *******************************************************************
// Method: Subject::Subject
//
// Purpose: 
//   Constructor for the Subject class.
//
// Programmer: Brad Whitlock
// Creation:   Wed Aug 9 16:00:53 PST 2000
//
// Modifications:
//   
// *******************************************************************

Subject::Subject()
{
}

// *******************************************************************
// Method: Subject::~Subject
//
// Purpose: 
//   Destructor for the Subject class.
//
// Programmer: Brad Whitlock
// Creation:   Wed Aug 9 16:00:53 PST 2000
//
// Modifications:
//   Brad Whitlock, Thu Aug 31 15:17:03 PST 2000
//   Added code to tell remaining observers not to try and detach
//   from the subject.
//
// *******************************************************************

Subject::~Subject()
{
    // If there are still observers around it means that this
    // object is being destroyed first. Tell the remaining observers
    // that they should not try to detach.
    std::vector<SimpleObserver *>::iterator pos;
    for(pos = observers.begin(); pos != observers.end(); ++pos)
    {
        (*pos)->SubjectRemoved(this);
    }
}

// *******************************************************************
// Method: Subject::Attach
//
// Purpose: 
//   Adds an Observer to the list of Observers that are watching the
//   Subject. When the subject changes, the new Observer will also
//   be called.
//
// Arguments:
//   o : A pointer to the new Observer.
//
// Programmer: Brad Whitlock
// Creation:   Wed Aug 9 16:02:26 PST 2000
//
// Modifications:
//   
// *******************************************************************

void
Subject::Attach(SimpleObserver *o)
{
    observers.push_back(o);
}

// *******************************************************************
// Method: Subject::Detach
//
// Purpose: 
//   Removes an Observer from the list of Observers that is maintained
//   by the subject. The detached observer will no longer be notified
//   when the subject changes.
//
// Arguments:
//   o : A pointer to the Observer that will be removed.
//
// Programmer: Brad Whitlock
// Creation:   Wed Aug 9 16:03:29 PST 2000
//
// Modifications:
//   
// *******************************************************************

void
Subject::Detach(SimpleObserver *o)
{
    std::vector<SimpleObserver *>::iterator pos;

    // Erase all references to observer o.
    for(pos = observers.begin(); pos != observers.end(); )
    {
       if(*pos == o)
           pos = observers.erase(pos);
       else
           ++pos;
    }
}

// *******************************************************************
// Method: Subject::Notify
//
// Purpose: 
//   Notifies all Observers that are watching the subject that the
//   subject has changed.
//
// Note:       
//   If an Observer's update state is false, that Observer is not
//   notified of the update because, presumeably, it was the Observer
//   that caused the subject to change.
//
// Programmer: Brad Whitlock
// Creation:   Wed Aug 9 16:04:43 PST 2000
//
// Modifications:
//   
// *******************************************************************

void
Subject::Notify()
{
    std::vector<SimpleObserver *>::iterator pos;

    for(pos = observers.begin(); pos != observers.end(); ++pos)
    {
        // Update the observer if it wants to be updated. If it didn't
        // want to be updated, set its update to true so it will be 
        // updated next time.
        if((*pos)->GetUpdate())
           (*pos)->Update(this);
        else
           (*pos)->SetUpdate(true);
    }
}
