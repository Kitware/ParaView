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

#include <AttributeSubject.h>

// *******************************************************************
// Method: AttributeSubject::AttributeSubject
//
// Purpose: 
//   Constructor for the AttributeSubject class.
//
// Arguments:
//   formatString : This string describes the kinds of attributes that
//                  are contained in the object.
//
// Programmer: Brad Whitlock
// Creation:   Mon Aug 7 12:53:39 PDT 2000
//
// Modifications:
//   
// *******************************************************************

AttributeSubject::AttributeSubject(const char *formatString) : 
    AttributeGroup(formatString), Subject()
{
    // nothing special here.
}

// *******************************************************************
// Method: AttributeSubject::~AttributeSubject
//
// Purpose: 
//   Destructor for the AttributeSubject class.
//
// Programmer: Brad Whitlock
// Creation:   Mon Aug 7 12:54:49 PDT 2000
//
// Modifications:
//   
// *******************************************************************

AttributeSubject::~AttributeSubject()
{
    // nothing special here either.
}

// *******************************************************************
// Method: AttributeSubject::Notify()
//
// Purpose: 
//   Tells all Observers to update, then unselects all the attributes.
//
// Programmer: Brad Whitlock
// Creation:   Wed Aug 9 15:55:06 PST 2000
//
// Modifications:
//   
// *******************************************************************

void
AttributeSubject::Notify()
{
    // Call the base class's Notify method.
    Subject::Notify();

    // Now that all the Obsevrers have been called, unselect all the
    // attributes.
    UnSelectAll();
}

// ****************************************************************************
// Method: AttributeSubject::CreateCompatible
//
// Purpose: 
//   Creates a compatible object of the specified type.
//
// Programmer: Brad Whitlock
// Creation:   Wed Oct 30 14:11:37 PST 2002
//
// Modifications:
//   
// ****************************************************************************

AttributeSubject *
AttributeSubject::CreateCompatible(const std::string &) const
{
    return 0;
}

// ****************************************************************************
// Method: AttributeSubject::TypeName
//
// Purpose: 
//   Returns the name of the type.
//
// Returns:    The name of the type.
//
// Programmer: Kathleen Bonnell 
// Creation:   June 20, 2006 
//
// Modifications:
//   
// ****************************************************************************

const std::string
AttributeSubject::TypeName() const
{
    return "AttributeSubject";
}

