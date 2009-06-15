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
//                            VisItException.h                               //
// ************************************************************************* //

#ifndef VISIT_EXCEPTION_H
#define VISIT_EXCEPTION_H

#include <exception>
#include <visitstream.h>
#include <string>
#ifdef FAKE_EXCEPTIONS
#include <setjmp.h>
#endif

#include <pqComponentsExport.h>

#ifdef FAKE_EXCEPTIONS
#define VISIT_THROW_NOTHING
#else
#define VISIT_THROW_NOTHING throw()
#endif

// ****************************************************************************
//  Class: VisItException
//
//  Purpose:
//      An abstract type that all VisIt exceptions should be derived from.
//
//  Note:   This is only a conceptionally abstract type, there are no
//          pure virtual functions.
//
//  Programmer: Hank Childs
//  Creation:   May 16, 2000
//
//  Modifications:
//    Jeremy Meredith, Fri Nov 17 17:23:00 PST 2000
//    Made log non-static, removed setLogFile, and removed InitVisItExceptions.
//
//    Kathleen Bonnell, Wed Apr 25 14:28:22 PDT 2001
//    Added a method to return the message.
//
//    Kathleen Bonnell, Fri Apr 27 10:43:22 PDT 2001
//    Added ability to set message via constructor.
//    Added a method to return the Exception type.
//
//    Brad Whitlock, Tue Oct 23 16:56:08 PST 2001
//    Added methods to return the filename and the line.
//
//    Kathleen Bonnell, Thu Nov  1 11:10:39 PST 2001
//    Removed const from GetLine to remove sgi compiler warning.
//
//    Brad Whitlock, Thu Mar 14 13:37:34 PST 2002
//    Added exports and a work-around for the windows compiler.
//
//    Brad Whitlock, Fri Oct 25 12:23:27 PDT 2002
//    I fixed a bug in the fake exceptions' TRY statement that could let
//    nested TRY's rethrow a previously caught exception.
//
//    Eric Brugger, Wed Jul 23 13:46:05 PDT 2003
//    No longer inherit from exception.
//
//    Brad Whitlock, Mon Aug 25 15:05:14 PST 2003
//    Added LogCatch.
//
//    Brad Whitlock, Mon Nov 1 16:13:22 PST 2004
//    Removed GetMessage method once and for all. Too many times has it
//    been confused with GetMessageA on Windows.
//
// ****************************************************************************

class VisItException
{
  public:
                       VisItException();
                       VisItException(const vtkstd::string &msg);
    virtual           ~VisItException() VISIT_THROW_NOTHING {;};

    void               Log(void);

    void               SetThrowLocation(int, const char *);
    void               SetType(const char *t) { type = t; };
    void               SetType(const vtkstd::string &t) { type = t; };
    const vtkstd::string &Message() { return msg; };
    const vtkstd::string &GetExceptionType() { return type; };

    int                GetLine() const { return line; };
    const vtkstd::string &GetFilename() const  { return filename; };

    static void LogCatch(const char *exceptionType, const char *srcFile,
                         int srcLine);

  protected:
    int                line;
    vtkstd::string        filename;
    vtkstd::string        msg;
    vtkstd::string        type;
    ostream           *log;
};

#ifndef FAKE_EXCEPTIONS
//
// This is the case where we can count on C++ to do exceptions for us.
//
#define EXCEPTION0(e) \
{\
      e _visit_exception;\
      _visit_exception.SetType(#e);\
      _visit_exception.SetThrowLocation(__LINE__, __FILE__);\
      _visit_exception.Log();\
      throw _visit_exception;\
}

#define EXCEPTION1(e, a1) \
{\
      e _visit_exception(a1);\
      _visit_exception.SetType(#e);\
      _visit_exception.SetThrowLocation(__LINE__, __FILE__);\
      _visit_exception.Log();\
      throw _visit_exception;\
}

#define EXCEPTION2(e, a1, a2) \
{\
      e _visit_exception(a1, a2);\
      _visit_exception.SetType(#e);\
      _visit_exception.SetThrowLocation(__LINE__, __FILE__);\
      _visit_exception.Log();\
      throw _visit_exception;\
}

#define EXCEPTION3(e, a1, a2, a3) \
{\
      e _visit_exception(a1, a2, a3);\
      _visit_exception.SetType(#e);\
      _visit_exception.SetThrowLocation(__LINE__, __FILE__);\
      _visit_exception.Log();\
      throw _visit_exception;\
}

#define RECONSTITUTE_EXCEPTION(E, M) \
{ \
      VisItException _visit_exception(M); \
      _visit_exception.SetType(E);\
      _visit_exception.SetThrowLocation(__LINE__, __FILE__);\
      _visit_exception.Log();\
      throw _visit_exception;\
}

#define TRY                 try {
#define CATCH(T)            } catch(T)   { VisItException::LogCatch(#T, __FILE__,  __LINE__);
#define CATCH2(T, A)        } catch(T &A) { VisItException::LogCatch(#T, __FILE__,  __LINE__);
#define CATCHALL(T)         } catch(...) { VisItException::LogCatch(#T, __FILE__,  __LINE__);
#define ENDTRY              }
#define RETHROW             throw
#define CATCH_RETURN(n)     return
#define CATCH_RETURN2(n, v) return (v)

#else
//
// We're simulating C++ exceptions.
//

// Macro to turn exception debugging on/off. When you want exception
// debugging output, add an A to the end of the line below.
#define EXPRINT(A)

extern int             jump_stack_top;
extern jmp_buf         jump_stack[100];
extern int             jump_retval;
extern bool            exception_caught;
extern VisItException *exception_object;

// Some prototypes that are used in the fake exception calls.
int  exception_lookup(const char *name);
bool exception_compatible(const char *name);
void exception_throw(int backup);
void exception_delete(bool condition);
int  exception_default_id();

// Use fake exceptions!
#define EXCEPTION0(e) \
{ \
    exception_object = new e; \
    exception_object->SetType(#e);\
    exception_object->SetThrowLocation(__LINE__, __FILE__);\
    exception_object->Log();\
    jump_retval = exception_lookup(#e); \
    exception_throw(0); \
}

#define EXCEPTION1(e, a1) \
{ \
    exception_object = new e(a1); \
    exception_object->SetType(#e);\
    exception_object->SetThrowLocation(__LINE__, __FILE__);\
    exception_object->Log();\
    jump_retval = exception_lookup(#e); \
    exception_throw(0); \
}

#define EXCEPTION2(e, a1, a2) \
{ \
    exception_object = new e(a1, a2); \
    exception_object->SetType(#e);\
    exception_object->SetThrowLocation(__LINE__, __FILE__);\
    exception_object->Log();\
    jump_retval = exception_lookup(#e); \
    exception_throw(0); \
}

#define EXCEPTION3(e, a1, a2, a3) \
{ \
    exception_object = new e(a1, a2, a3); \
    exception_object->SetType(#e);\
    exception_object->SetThrowLocation(__LINE__, __FILE__);\
    exception_object->Log();\
    jump_retval = exception_lookup(#e); \
    exception_throw(0); \
}

#define RECONSTITUTE_EXCEPTION(E, M) \
{ \
    exception_object = new VisItException(M); \
    exception_object->SetType(E);\
    exception_object->SetThrowLocation(__LINE__, __FILE__);\
    exception_object->Log();\
    jump_retval = exception_default_id(); \
    exception_throw(0); \
}

#define TRY \
    exception_delete(true); \
    ++jump_stack_top; \
    jump_retval = setjmp(jump_stack[jump_stack_top]); \
    exception_caught = false; \
    EXPRINT(debug1 << "TRY("<<jump_stack_top<<"): "<<__FILE__<<":"<<__LINE__<<" jump_retval = " << jump_retval << endl;) \
    if(jump_retval == 0) \
    {


#define CATCH(e) \
        exception_delete(exception_caught && (jump_retval != 0)); \
    } \
    else if(exception_compatible(#e)) \
    { \
        VisItException::LogCatch(#e, __FILE__,  __LINE__); \
        exception_caught = true;


#define CATCH2(e, N) \
        exception_delete(exception_caught && (jump_retval != 0)); \
    } \
    else if(exception_compatible(#e)) \
    { \
        VisItException::LogCatch(#e, __FILE__,  __LINE__); \
        e & N = *((e *)exception_object); \
        exception_caught = true;


#define CATCHALL(e) \
        exception_delete(jump_retval != 0); \
    } \
    else \
    { \
        VisItException::LogCatch(#e, __FILE__,  __LINE__); \
        exception_caught = true; \


#define ENDTRY  \
        exception_delete(exception_caught); \
    } \
    if(exception_object != 0) \
    { \
        EXPRINT(debug1 << "Uncaught " << exception_object->GetExceptionType() \
                       <<" exception at (" << __FILE__ << ":" \
                       << __LINE__ << ") top="<<jump_stack_top << endl;) \
        exception_throw(1); \
    } \
    --jump_stack_top; \
    EXPRINT(debug1 << "ENDTRY: jump_stack_top = " << jump_stack_top << endl;) \

#define RETHROW \
    if(exception_object != 0) \
        exception_throw(1);

#define CATCH_RETURN(n) \
{ \
    exception_delete(exception_caught); \
    jump_stack_top -= (n); \
    return; \
}

#define CATCH_RETURN2(n, v) \
{ \
    exception_delete(exception_caught); \
    jump_stack_top -= (n); \
    return (v); \
}

#endif

#endif


