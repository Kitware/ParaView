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

// ************************************************************************* //
//                              VisItException.C                             //
// ************************************************************************* //

#include <VisItException.h>

// ****************************************************************************
//  Method:  VisItException constructor
//
//  Programmer: Hank Childs
//  Creation:   May 16, 2000
//
//  Modifications:
//    Jeremy Meredith, Fri Nov 17 17:22:28 PST 2000
//    Added initialization of log since it is no longer static.
//
// ****************************************************************************

VisItException::VisItException()
{
    filename = "Unknown";
    msg      = "Not set";
    type     = "VisItException";
    line     = -1;
    log      = NULL; //&debug1_real;
}

// ****************************************************************************
//  Method:  VisItException constructor
//
//  Programmer: Kathleen Bonnell 
//  Creation:   April 27, 2001 
//
//  Modifications:
//  
//    Mark C. Miller, Sun Dec  3 12:20:11 PST 2006
//    Added component name to exception string
//
//    Mark C. Miller, Thu May 24 20:29:35 PDT 2007
//    Added filtering for component name
//
// ****************************************************************************

VisItException::VisItException(const vtkstd::string &m)
{
    filename = "Unknown";
    vtkstd::string mtmp = vtkstd::string(m,0,16);
    if (mtmp.find(':') == vtkstd::string::npos)
        msg = /*vtkstd::string(Init::GetComponentName())*/ + ": " + m;
    else
    {
        if (mtmp.find("avtprep:") == 0 ||
            mtmp.find("cli:") == 0 ||
            mtmp.find("engine:") == 0 ||
      mtmp.find("gui:") == 0 ||
            mtmp.find("launcher:") == 0 ||
            mtmp.find("mdserver:") == 0 ||
      mtmp.find("viewer:") == 0)
            msg = m;
        else
            msg = /*vtkstd::string(Init::GetComponentName())*/ + ": " + m;
    }
    type     = "VisItException";
    line     = -1;
    log      = NULL; //&debug1_real;
}


// ****************************************************************************
//  Method: VisItException::SetThrowLocation
//
//  Purpose:
//      Sets the location that the exception will be thrown from.  This is
//      called by the THROW macro.
//
//  Arguments:
//      l           The line number that the exception will be thrown from.
//      f           The file name that the exception will be thrown from.
//
//  Programmer: Hank Childs
//  Creation:   May 16, 2000
//
// ****************************************************************************

void
VisItException::SetThrowLocation(int l, const char *f)
{
    line     = l;
    filename = f;
}


// ****************************************************************************
//  Method: VisItException::Log
//
//  Purpose:
//      Outputs the string to the log file.
//
//  Programmer: Hank Childs
//  Creation:   May 16, 2000
//
// ****************************************************************************

void
VisItException::Log(void)
{
    if (log == NULL)
    {
        return;
    }

    //
    // Don't want something funny to happen when trying to log an exception,
    // so wrap this around a try and catch any exception it might throw.
    //
#ifdef FAKE_EXCEPTIONS
    (*log) << "(" << type << ") " << filename << ", line " << line << ": " 
           << msg << endl;
#else
    try
    {
        (*log) << "(" << type.c_str() << ") " << filename.c_str() << ", line " << line << ": " 
               << msg.c_str() << endl;
    }
    catch(...)
    {
        ;
    }
#endif
}

// ****************************************************************************
// Method: VisItException::LogCatch
//
// Purpose: 
//   Saves information about a caught exception to the log file.
//
// Arguments:
//   exceptionName : The name of the exception being caught.
//   srcFile       : The source file where the exception was caught.
//   srcLine       : The source line where the exception was caught.
//
// Programmer: Brad Whitlock
// Creation:   Mon Aug 25 15:15:26 PST 2003
//
// Modifications:
//   
// ****************************************************************************

void
VisItException::LogCatch(
  const char* /*exceptionName*/, const char* /*srcFile*/, int /*srcLine*/)
{
//    debug1_real << "catch(" << exceptionName << ") " << srcFile << ":" << srcLine << endl;
}

#ifdef FAKE_EXCEPTIONS
// ****************************************************************************
//          Fake C++ exception code that gets us by on sucky platforms.
// ****************************************************************************

// Exception state variables.
int             jump_stack_top = -1;
jmp_buf         jump_stack[100];
int             jump_retval;
bool            exception_caught = false;
VisItException *exception_object;

typedef struct
{
    const char *name;
    const char *parent_name;
} exception_info;

//
// Make sure this list stays alphabetized by exception name.
//
// Modifications:
//
//   Hank Childs, Thu May 23 14:08:38 PDT 2002
//   Added BadCellException.
//
//   Hank Childs, Tue May 28 10:17:47 PDT 2002
//   Added NoCurveException.
//
//   Brad Whitlock, Fri Jun 28 13:28:21 PST 2002
//   Added ExpressionException.
//
//   Brad Whitlock, Fri Jul 26 11:18:05 PDT 2002
//   Added FileDoesNotExistException.
//
//   Kathleen Bonnell, Fri Aug 16 16:37:08 PDT 2002 
//   Added GhostCellException, InvalidCategoryException, 
//   InvalidSetException and LogicalIndexException.
//
//   Hank Childs, Wed Aug 28 16:41:20 PDT 2002
//   Added NoDefaultVariableException.
//
//   Brad Whitlock, Oct 3 11:11:23 PDT 2002
//   Added CancelledConnectException.
//
//   Hank Childs, Mon Oct 14 09:19:12 PDT 2002
//   Added InvalidDBTypeException.
//
//   Kathleen Bonnell, Wed Oct 23 15:11:44 PDT 2002 
//   Added NonQueryableInputException.
//
//   Brad Whitlock, Fri Dec 27 12:33:05 PDT 2002
//   I added IncompatibleSecurityTokenException.
//
//   Kathleen Bonnell, Thu Mar 13 11:22:05 PST 2003  
//   Added NoEngineException.
//
//   Brad Whitlock, Fri Apr 25 10:29:45 PDT 2003
//   Added InvalidColortableException.
//
//   Kathleen Bonnell, Tue May 20 14:25:11 PDT 2003  
//   Added BadVectorException. 
//
//   Jeremy Meredith, Fri Aug 15 11:19:25 PDT 2003
//   Added RecursiveExpressionException and InvalidExpressionException.
//
//   Kathleen Bonnell, Tue Dec 16 14:58:46 PST 2003 
//   Added UnexpectedValueException.
//
//   Kathleen Bonnell, Tue Jan 13 08:48:14 PST 2004 
//   Added BadNodeException.
//
//   Mark C. Miller, Mon Apr 19 11:41:07 PDT 2004
//   Added PlotterException base class for plotter exceptions
//
//   Mark C. Miller, Sun Dec  3 12:20:11 PST 2006
//   Added PointerNotInCacheException for transform manager

static const exception_info exception_tree[] =
{
    // Exception name                      Parent Exception name
    {"AbortException",                     "PipelineException"},
    {"BadCellException",                   "PipelineException"},
    {"BadColleagueException",              "VisWindowException"},
    {"BadDomainException",                 "PipelineException"},
    {"BadHostException",                   "VisItException"},
    {"BadIndexException",                  "PipelineException"},
    {"BadInteractorException",             "VisWindowException"},
    {"BadNodeException",                   "PipelineException"},
    {"BadPermissionException",             "DatabaseException"},
    {"BadPlotException",                   "VisWindowException"},
    {"BadWindowModeException",             "VisWindowException"},
    {"BadVectorException",                 "PipelineException"},
    {"CancelledConnectException",          "VisItException"},
    {"ChangeDirectoryException",           "VisItException"},
    {"CouldNotConnectException",           "VisItException"},
    {"DatabaseException",                  "VisItException"},
    {"ExpressionException",                "PipelineException"},
    {"FileDoesNotExistException",          "DatabaseException"},
    {"GetFileListException",               "VisItException"},
    {"GetMetaDataException",               "VisItException"},
    {"GhostCellException",                 "PipelineException"},
    {"ImproperUseException",               "PipelineException"},
    {"IncompatibleDomainListsException",   "PipelineException"},
    {"IncompatibleSecurityTokenException", "VisItException"},
    {"IncompatibleVersionException",       "VisItException"},
    {"IntervalTreeNotCalculatedException", "PipelineException"},
    {"InvalidCategoryException",           "PipelineException"},
    {"InvalidCellTypeException",           "PipelineException"},
    {"InvalidColortableException",         "PlotterException"},
    {"InvalidDBTypeException",             "DatabaseException"},
    {"InvalidDimensionsException",         "PipelineException"},
    {"InvalidDirectoryException",          "VisItException"},
    {"InvalidExpressionException",         "VisItException"},
    {"InvalidFilesException",              "DatabaseException"},
    {"InvalidLimitsException",             "PipelineException"},
    {"InvalidMergeException",              "PipelineException"},
    {"InvalidPluginException",             "VisItException"},
    {"InvalidSetException",                "PipelineException"},
    {"InvalidSourceException",             "DatabaseException"},
    {"InvalidTimeStepException",           "DatabaseException"},
    {"InvalidVariableException",           "DatabaseException"},
    {"InvalidZoneTypeException",           "DatabaseException"},
    {"LogicalIndexException",              "PipelineException"},
    {"LostConnectionException",            "VisItException"},
    {"NoCurveException",                   "PipelineException"},
    {"NoDefaultVariableException",         "PipelineException"},
    {"NoEngineException",                  "VisItException"},
    {"NoInputException",                   "PipelineException"},
    {"NonQueryableInputException",         "PipelineException"},
    {"PipelineException",                  "VisItException"},
    {"PlotDimensionalityException",        "VisWindowException"},
    {"RecursiveExpressionException",       "VisItException"},
    {"SiloException",                      "DatabaseException"},
    {"StubReferencedException",            "PipelineException"},
    {"UnexpectedValueException",           "PipelineException"},
    {"VisItException",                     NULL},
    {"VisWindowException",                 "VisItException"},
    {"PointerNotInCacheException",         "DatabaseException"}
};

//
// The number of exceptions in the table.
//
static const int num_exception_names = sizeof(exception_tree)/(2*sizeof(const char *));

// ****************************************************************************
// Function: recursive_exception_lookup
//
// Purpose:
//   This function does a binary search through the exception info to get the
//   index that contains the specified exception name.
//
// Notes:      
//
// Programmer: Brad Whitlock
// Creation:   Mon Oct 22 20:55:05 PST 2001
//
// Modifications:
//   
// ****************************************************************************

static int
recursive_exception_lookup(const char *name, int start, int end)
{
    int retval = start;

    int delta = (end - start) >> 1;
    int pivot = start + delta;
    int comp = strcmp(name, exception_tree[pivot].name);

    if(delta == 0)
        retval = (comp == 0) ? pivot : -1;
    else if(comp < 0)
        retval = recursive_exception_lookup(name, start, pivot);
    else if(comp == 0)
        retval = (comp == 0) ? pivot : -1;
    else
        retval = recursive_exception_lookup(name, pivot, end);

    return retval;
}

// ****************************************************************************
// Function: exception_lookup
//
// Purpose: 
//   Returns the arbitrary id that is associated with an exception name.
//
// Arguments:
//   name : The name of the exception that we're looking up.
//
// Programmer: Brad Whitlock
// Creation:   Mon Oct 22 19:22:51 PST 2001
//
// Modifications:
//   
// ****************************************************************************

int
exception_lookup(const char *name)
{
    int retval = recursive_exception_lookup(name, 0, num_exception_names);

    if(retval < 0)
    {
        debug1 << "exception_lookup: index < 0. This means that the fake "
                  "exception table contains errors! Exception name = "
               << name << endl;
        abort();
    }

    // Make the minimum value returned 100.
    return retval + 100;
}

// ****************************************************************************
// Function: exception_default_id
//
// Purpose: 
//   Returns the id for VisItException.
//
// Returns:    The id for VisItException.
//
// Programmer: Brad Whitlock
// Creation:   Tue Oct 23 14:27:03 PST 2001
//
// Modifications:
//   
// ****************************************************************************

int
exception_default_id()
{
    return exception_lookup("VisItException");
}

// ****************************************************************************
// Function: exception_compatible
//
// Purpose: 
//   This function determines if the specified exception type is compatible
//   with the current exception. This is how exception inheritance is being
//   handled.
//
// Arguments:
//   name : The name of the exception that we're testing against.
//
// Programmer: Brad Whitlock
// Creation:   Mon Oct 22 19:24:17 PST 2001
//
// Modifications:
//   
// ****************************************************************************

bool
exception_compatible(const char *name)
{
    bool compatible = false;
    bool noErrors = true;

    // If the jump_retval variable is zero then we do not have an exception.
    if(jump_retval != 0)
    {
        int index = jump_retval - 100;
        const char *exeception_name = exception_tree[index].name;

        if(strcmp(exeception_name, name) == 0)
            compatible = true;
        else
        {
            // We have to traverse up the inheritance hierarchy to find a
            // compatible exception type.
            do
            {
                const char *eptr = exception_tree[index].parent_name;

                if(eptr == NULL)
                {
                    // There is no parent for this exception, say that
                    // it is compatible to get out of the loop, but set the
                    // error flag.
                    compatible = true;
                    noErrors = false;
                }
                else
                { 
                    compatible = (strcmp(eptr, name) == 0);
                    if(!compatible)
                        index = exception_lookup(eptr) - 100;
                }
            } while(!compatible);
        }
    }

    return (compatible && noErrors);
}

// ****************************************************************************
// Function: exception_propagate
//
// Purpose: 
//   Does a longjmp to the top of the jump stack minus the backup amount. If
//   there are not enough try's on the stack it is an unhandled exception and
//   we abort the program.
//
// Arguments:
//   backup : The amount of jumps to backup.
//
// Programmer: Brad Whitlock
// Creation:   Mon Oct 22 21:44:14 PST 2001
//
// Modifications:
//   
// ****************************************************************************

void
exception_throw(int backup)
{
    EXPRINT(debug1 << "throw(" << exception_object->GetExceptionType() << ") at "
                   << exception_object->GetFilename() << ":"
                   << exception_object->GetLine()
                   << ", backup = " << backup << endl;)

    if(jump_stack_top < backup)
    {
        EXPRINT(debug1 << "There are no more TRY's on the exception stack! Aborting." << endl;)
        abort();
    }
    else
    {
        jump_stack_top -= backup;
        EXPRINT(debug1 << "exception_throw: Throwing exception "
                       << exception_object->GetExceptionType()
                       << " by jumping to TRY[" << jump_stack_top << "]" << endl;)
        longjmp(jump_stack[jump_stack_top], jump_retval);
    }
}

// ****************************************************************************
// Function: exception_delete
//
// Purpose:
//   This function deletes the exception object based on a condition.
//
// Notes:      
//
// Programmer: Brad Whitlock
// Creation:   Mon Oct 22 21:50:08 PST 2001
//
// Modifications:
//   Brad Whitlock, Fri Oct 25 12:22:37 PDT 2002
//   I added code to prevent the exception from being deleted if it is
//   already deleted.
//
// ****************************************************************************

void
exception_delete(bool condition)
{
    if(condition && exception_object != 0)
    {
        EXPRINT(debug1 << "exception_delete()" << endl;)
        delete exception_object;
        exception_object = 0;
    }
}

#endif
