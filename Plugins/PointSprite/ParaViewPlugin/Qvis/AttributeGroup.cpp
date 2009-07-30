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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <AttributeGroup.h>
#include <EqualVal.h>
#include <Interpolator.h>

//
// These constants represent the types of attributes.
//
static const unsigned char msgTypeNone           = 0x00;
static const unsigned char msgTypeChar           = 0x01;
static const unsigned char msgTypeUnsignedChar   = 0x02;
static const unsigned char msgTypeInt            = 0x03;
static const unsigned char msgTypeLong           = 0x04;
static const unsigned char msgTypeFloat          = 0x05;
static const unsigned char msgTypeDouble         = 0x06;
static const unsigned char msgTypeString         = 0x07;
static const unsigned char msgTypeAttributeGroup = 0x08;
static const unsigned char msgTypeBool           = 0x09;

static const unsigned char msgTypeListChar           = 0x0a;
static const unsigned char msgTypeListUnsignedChar   = 0x0b;
static const unsigned char msgTypeListInt            = 0x0c;
static const unsigned char msgTypeListLong           = 0x0d;
static const unsigned char msgTypeListFloat          = 0x0e;
static const unsigned char msgTypeListDouble         = 0x0f;
static const unsigned char msgTypeListString         = 0x10;
static const unsigned char msgTypeListAttributeGroup = 0x11;
static const unsigned char msgTypeListBool           = 0x12;

static const unsigned char msgTypeVectorChar           = 0x13;
static const unsigned char msgTypeVectorUnsignedChar   = 0x14;
static const unsigned char msgTypeVectorInt            = 0x15;
static const unsigned char msgTypeVectorLong           = 0x16;
static const unsigned char msgTypeVectorFloat          = 0x17;
static const unsigned char msgTypeVectorDouble         = 0x18;
static const unsigned char msgTypeVectorString         = 0x19;
static const unsigned char msgTypeVectorAttributeGroup = 0x1a;
static const unsigned char msgTypeVectorBool           = 0x1b;

#if 0
// These are uesful for creating debugging output. Ordinarily, these
// are not needed so they are ifdef'd out.
static const char *typeNames[] = {
"None",
"char", "unsigned char", "int", "long", "float", "double", "string", "AttributeGroup", "bool",
"ListChar", "ListUnsignedChar", "ListInt", "ListLong", "ListFloat",
"ListDouble", "ListString", "ListAttributeGroup", "ListBool",
"VectorChar", "VectorUnsignedChar", "VectorInt", "VectorLong", "VectorFloat",
"VectorDouble", "VectorString", "VectorAttributeGroup", "VectorBool",
};
#endif

// ****************************************************************************
// Method: AttributeGroup::AttributeGroup
//
// Purpose:
//   This is the constructor for the AttributeGroup class. It is
//   responsible for creating the type map that will allow the
//   sub class to be serialized onto a Connection object.
//
// Arguments:
//   formatString : A NULL-terminated character string where each
//                  character denotes the type of the attribute being
//                  stored in the AttributeGroup. Valid characters
//                  and their meaning are listed below:
//
//                  c = char,           C = list of char
//                  u = unsinged char,  U = list of unsigned char
//                  i = int,            I = list of int
//                  l = long,           L = list of long
//                  f = float,          F = list of float
//                  d = double,         D = list of double
//                  a = AttributeGroup, A = list of AttributeGroup
//                  b = bool,           B = list of bool
// Returns:
//
// Note:
//
// Programmer: Brad Whitlock
// Creation:   Fri Aug 4 16:15:57 PST 2000
//
// Modifications:
//    Jeremy Meredith, Mon Feb 26 16:02:50 PST 2001
//    Added unsigned chars.
//
// ****************************************************************************

AttributeGroup::AttributeGroup(const char *formatString) : typeMap()
{
    guido = -1;
    CreateTypeMap(formatString);
}

// ****************************************************************************
// Method: AttributeGroup::~AttributeGroup
//
// Purpose:
//   Destructor for the AttributeGroup class.
//
// Programmer: Brad Whitlock
// Creation:   Fri Aug 4 16:30:26 PST 2000
//
// Modifications:
//
// ****************************************************************************

AttributeGroup::~AttributeGroup()
{

}

// ****************************************************************************
// Method: AttributeGroup::NumAttributes
//
// Purpose:
//   Returns the number of attributes in the AttributeGroup. This is
//   useful for iteration over the AttributeGroup.
//
// Programmer: Brad Whitlock
// Creation:   Fri Aug 4 16:30:47 PST 2000
//
// Modifications:
//
// ****************************************************************************

int
AttributeGroup::NumAttributes() const
{
    return static_cast<int>(typeMap.size());
}

// ****************************************************************************
// Method: AttributeGroup::IsSelected
//
// Purpose:
//   Returns whether or not a particular attribute is selected. If it
//   is selected, then a value has been changed recently, or the
//   object is being prepared for transmission across a Connection.
//
// Arguments:
//   i : The index of the attribute that is to be checked.
//
// Returns:
//   If it is not in the range [0..NumAttributes()-1], the return value
//   is false. Otherwise, the value returned is whether or no the
//   attribute is selected.
//
// Note:
//
// Programmer: Brad Whitlock
// Creation:   Fri Aug 4 16:31:38 PST 2000
//
// Modifications:
//
// ****************************************************************************

bool
AttributeGroup::IsSelected(int i) const
{
    bool retval = false;

    // If the index is valid, check the selected flag.
    if(i >= 0 && i < static_cast<int>(typeMap.size()))
    {
         retval = typeMap[i].selected;
    }

    return retval;
}

// ****************************************************************************
// Method: AttributeGroup::CopyAttributes
//
// Purpose:
//   Copies the attributes into the current object and returns whether or not
//   the attributes were copied.
//
// Arguments:
//   atts : The attributes that we want to copy into the current object.
//
// Programmer: Brad Whitlock
// Creation:   Tue Oct 9 15:40:55 PST 2001
//
// Modifications:
//
// ****************************************************************************

bool
AttributeGroup::CopyAttributes(const AttributeGroup * /*atts*/)
{
    return false;
}

// ****************************************************************************
// Method: AttributeGroup::InterpolateConst
//
// Purpose:
//   Set the current attribute group by interpolating between the two specified
//   attribute groups.
//
// Arguments:
//   atts1 : The first attribute group to interpolate between.
//   atts2 : The second attribute group to interpolate between.
//   f     : The fraction to interpolate between the two attribute groups.
//
// Programmer: Jeremy Meredith
// Creation:   January 17, 2003
//
// Modifications:
//    Jeremy Meredith, Thu Jan 23 13:31:27 PST 2003
//    Modified the attribute group vector interpolation.  It resizes the
//    output vector here and calls the normal interpolator on it.
//    Fixed "color" type as well.
//
//    Jeremy Meredith, Fri Jan 31 09:48:03 PST 2003
//    Made opacity a double.
//
//    Brad Whitlock, Thu Dec 9 15:07:10 PST 2004
//    Added variablename type.
//
//    Kathleen Bonnell, Thu Mar 22 16:43:38 PDT 2007
//    Added scalemode type.
//
// ****************************************************************************

void
AttributeGroup::InterpolateConst(const AttributeGroup *atts1,
                                 const AttributeGroup *atts2, double f)
{
    SelectAll();
    int n = NumAttributes();

    for (int i=0; i<n; i++)
    {
        if (!typeMap[i].selected)
            continue;

        void *addrOut = typeMap[i].address;
        void *addr1   = atts1->typeMap[i].address;
        void *addr2   = atts2->typeMap[i].address;
        int   length  = typeMap[i].length;

        switch (GetFieldType(i))
        {
          case FieldType_int:
            ConstInterp<int>::InterpScalar(addrOut,addr1,addr2,f);
            break;
          case FieldType_intArray:
            ConstInterp<int>::InterpArray(addrOut,addr1,addr2,length, f);
            break;
          case FieldType_intVector:
            ConstInterp<int>::InterpVector(addrOut,addr1,addr2,f);
            break;
          case FieldType_bool:
            ConstInterp<bool>::InterpScalar(addrOut,addr1,addr2,f);
            break;
          case FieldType_boolVector:
            ConstInterp<bool>::InterpVector(addrOut,addr1,addr2,f);
            break;
          case FieldType_float:
            ConstInterp<float>::InterpScalar(addrOut,addr1,addr2,f);
            break;
          case FieldType_floatArray:
            ConstInterp<float>::InterpArray(addrOut,addr1,addr2,length, f);
            break;
          case FieldType_double:
            ConstInterp<double>::InterpScalar(addrOut,addr1,addr2,f);
            break;
          case FieldType_doubleArray:
            ConstInterp<double>::InterpArray(addrOut,addr1,addr2,length, f);
            break;
          case FieldType_doubleVector:
            ConstInterp<double>::InterpVector(addrOut,addr1,addr2,f);
            break;
          case FieldType_uchar:
            ConstInterp<unsigned char>::InterpScalar(addrOut,addr1,addr2,f);
            break;
          case FieldType_ucharArray:
            ConstInterp<unsigned char>::InterpArray(addrOut,addr1,addr2,length, f);
            break;
          case FieldType_ucharVector:
            ConstInterp<unsigned char>::InterpVector(addrOut,addr1,addr2,f);
            break;
          case FieldType_string:
            ConstInterp<vtkstd::string>::InterpScalar(addrOut,addr1,addr2,f);
            break;
          case FieldType_stringVector:
            ConstInterp<vtkstd::string>::InterpVector(addrOut,addr1,addr2,f);
            break;
          case FieldType_colortable:
            ConstInterp<vtkstd::string>::InterpScalar(addrOut,addr1,addr2,f);
            break;
          case FieldType_opacity:
            ConstInterp<double>::InterpScalar(addrOut,addr1,addr2,f);
            break;
          case FieldType_linestyle:
            ConstInterp<int>::InterpScalar(addrOut,addr1,addr2,f);
            break;
          case FieldType_linewidth:
            ConstInterp<int>::InterpScalar(addrOut,addr1,addr2,f);
            break;
          case FieldType_variablename:
            ConstInterp<vtkstd::string>::InterpScalar(addrOut,addr1,addr2,f);
            break;
          case FieldType_color:
          case FieldType_att:
            ((AttributeGroup*)addrOut)->
                                   InterpolateConst((AttributeGroup*)addr1,
                                                    (AttributeGroup*)addr2, f);
            break;
          case FieldType_attVector:
            {
                AttributeGroupVector &out=*(AttributeGroupVector*)addrOut;
                AttributeGroupVector &a1 =*(AttributeGroupVector*)addr1;
                AttributeGroupVector &a2 =*(AttributeGroupVector*)addr2;
                int l0 = static_cast<int>(out.size());
                int l1 = static_cast<int>(a1.size());
                int l2 = static_cast<int>(a2.size());
                int lmax = (l1 > l2) ? l1 : l2;
                out.resize(lmax);
                if (lmax > l0)
                {
                    for (int j=l0; j<lmax; j++)
                    {
                        out[j] = CreateSubAttributeGroup(i);
                    }
                }
                ConstInterp<AttributeGroup*>::InterpVector(&out, &a1, &a2, f);
            }
            break;
          case FieldType_enum:
            ConstInterp<int>::InterpScalar(addrOut,addr1,addr2,f);
            break;
          case FieldType_scalemode:
            ConstInterp<int>::InterpScalar(addrOut,addr1,addr2,f);
            break;
          default:
            cerr << "UNKNOWN TYPE IN AttributeGroup::InterpolateConst\n";
            break;
        }
    }
}

// ****************************************************************************
// Method: AttributeGroup::InterpolateLinear
//
// Purpose:
//   Set the current attribute group by interpolating between the two specified
//   attribute groups.
//
// Arguments:
//   atts1 : The first attribute group to interpolate between.
//   atts2 : The second attribute group to interpolate between.
//   f     : The fraction to interpolate between the two attribute groups.
//
// Programmer: Jeremy Meredith
// Creation:   January 17, 2003
//
// Modifications:
//    Jeremy Meredith, Thu Jan 23 13:31:27 PST 2003
//    Modified the attribute group vector interpolation.  It resizes the
//    output vector here and calls the normal interpolator on it.
//    Fixed "color" type as well.
//
//    Jeremy Meredith, Fri Jan 31 09:47:40 PST 2003
//    Made opacity a double, and made line width interpolate linearly.
//
//    Brad Whitlock, Thu Dec 9 15:07:46 PST 2004
//    Added variablename type.
//
//    Hank Childs, Fri May 19 16:33:41 PDT 2006
//    Fix crash that can come when this method is called and attributes have
//    not been fully initialized.
//
//    Kathleen Bonnell, Thu Mar 22 16:43:38 PDT 2007
//    Added scalemode type.
//
// ****************************************************************************

void
AttributeGroup::InterpolateLinear(const AttributeGroup *atts1,
                                  const AttributeGroup *atts2, double f)
{
    SelectAll();
    int n = NumAttributes();

    for (int i=0; i<n; i++)
    {
        if (!typeMap[i].selected)
            continue;

        void *addrOut = typeMap[i].address;
        void *addr1   = atts1->typeMap[i].address;
        void *addr2   = atts2->typeMap[i].address;
        int   length  = typeMap[i].length;

        if (addrOut == NULL || addr1 == NULL || addr2 == NULL)
            continue;

        switch (GetFieldType(i))
        {
          case FieldType_int:
            LinInterp<int>::InterpScalar(addrOut,addr1,addr2,f);
            break;
          case FieldType_intArray:
            LinInterp<int>::InterpArray(addrOut,addr1,addr2,length, f);
            break;
          case FieldType_intVector:
            LinInterp<int>::InterpVector(addrOut,addr1,addr2,f);
            break;
          case FieldType_bool:
            ConstInterp<bool>::InterpScalar(addrOut,addr1,addr2,f);
            break;
          case FieldType_boolVector:
            ConstInterp<bool>::InterpVector(addrOut,addr1,addr2,f);
            break;
          case FieldType_float:
            LinInterp<float>::InterpScalar(addrOut,addr1,addr2,f);
            break;
          case FieldType_floatArray:
            LinInterp<float>::InterpArray(addrOut,addr1,addr2,length, f);
            break;
          case FieldType_double:
            LinInterp<double>::InterpScalar(addrOut,addr1,addr2,f);
            break;
          case FieldType_doubleArray:
            LinInterp<double>::InterpArray(addrOut,addr1,addr2,length, f);
            break;
          case FieldType_doubleVector:
            LinInterp<double>::InterpVector(addrOut,addr1,addr2,f);
            break;
          case FieldType_uchar:
            LinInterp<unsigned char>::InterpScalar(addrOut,addr1,addr2,f);
            break;
          case FieldType_ucharArray:
            LinInterp<unsigned char>::InterpArray(addrOut,addr1,addr2,length, f);
            break;
          case FieldType_ucharVector:
            LinInterp<unsigned char>::InterpVector(addrOut,addr1,addr2,f);
            break;
          case FieldType_string:
            ConstInterp<vtkstd::string>::InterpScalar(addrOut,addr1,addr2,f);
            break;
          case FieldType_stringVector:
            ConstInterp<vtkstd::string>::InterpVector(addrOut,addr1,addr2,f);
            break;
          case FieldType_colortable:
            ConstInterp<vtkstd::string>::InterpScalar(addrOut,addr1,addr2,f);
            break;
          case FieldType_opacity:
            LinInterp<double>::InterpScalar(addrOut,addr1,addr2,f);
            break;
          case FieldType_linestyle:
            ConstInterp<int>::InterpScalar(addrOut,addr1,addr2,f);
            break;
          case FieldType_linewidth:
            LinInterp<int>::InterpScalar(addrOut,addr1,addr2,f);
            break;
          case FieldType_variablename:
            ConstInterp<vtkstd::string>::InterpScalar(addrOut,addr1,addr2,f);
            break;
          case FieldType_color:
          case FieldType_att:
            ((AttributeGroup*)addrOut)->
                                  InterpolateLinear((AttributeGroup*)addr1,
                                                    (AttributeGroup*)addr2, f);
            break;
          case FieldType_attVector:
            {
                AttributeGroupVector &out=*(AttributeGroupVector*)addrOut;
                AttributeGroupVector &a1 =*(AttributeGroupVector*)addr1;
                AttributeGroupVector &a2 =*(AttributeGroupVector*)addr2;
                int l0 = static_cast<int>(out.size());
                int l1 = static_cast<int>(a1.size());
                int l2 = static_cast<int>(a2.size());
                int lmax = (l1 > l2) ? l1 : l2;
                out.resize(lmax);
                if (lmax > l0)
                {
                    for (int j=l0; j<lmax; j++)
                    {
                        out[j] = CreateSubAttributeGroup(i);
                    }
                }
                LinInterp<AttributeGroup*>::InterpVector(&out, &a1, &a2, f);
            }
            break;
          case FieldType_enum:
            ConstInterp<int>::InterpScalar(addrOut,addr1,addr2,f);
            break;
          case FieldType_scalemode:
            ConstInterp<int>::InterpScalar(addrOut,addr1,addr2,f);
            break;
          default:
            cerr << "UNKNOWN TYPE IN AttributeGroup::InterpolateLinear\n";
            break;
        }
    }
}

// ****************************************************************************
// Method: AttributeGroup::EqualTo
//
// Purpose:
//   Determine if 'this' attribute group is equal to the one passed in
//
// Programmer: Mark C. Miller
// Creation:   06May03
//
// Modified:
//    Jeremy Meredith, Wed May 21 12:59:48 PDT 2003
//    Added missing break statements.  Added code to make sure
//    we're the same type of object before attempting any
//    comparisons.
//
//    Brad Whitlock, Thu Dec 9 15:08:23 PST 2004
//    Added variablename type.
//
//    Kathleen Bonnell, Thu Mar 22 16:43:38 PDT 2007
//    Added scalemode type.
//
// ****************************************************************************

bool
AttributeGroup::EqualTo(const AttributeGroup *atts) const
{
    // return immediately if its the same object
    if (this == atts)
       return true;

    // return immediately if it's a different type of object
    if (TypeName() != atts->TypeName())
        return false;

    int n = NumAttributes();

    for (int i=0; i<n; i++)
    {
        void *addr1   =       typeMap[i].address;
        void *addr2   = atts->typeMap[i].address;
        int   length  = typeMap[i].length;

        switch (GetFieldType(i))
        {
          case FieldType_int:
            if (!(EqualVal<int>::EqualScalar(addr1,addr2)))
               return false;
            break;
          case FieldType_intArray:
            if (!(EqualVal<int>::EqualArray(addr1,addr2,length)))
               return false;
            break;
          case FieldType_intVector:
            if (!(EqualVal<int>::EqualVector(addr1,addr2)))
               return false;
            break;
          case FieldType_bool:
            if (!(EqualVal<bool>::EqualScalar(addr1,addr2)))
               return false;
            break;
          case FieldType_boolVector:
            if (!(EqualVal<bool>::EqualVector(addr1,addr2)))
               return false;
            break;
          case FieldType_float:
            if (!(EqualVal<float>::EqualScalar(addr1,addr2)))
               return false;
            break;
          case FieldType_floatArray:
            if (!(EqualVal<float>::EqualArray(addr1,addr2,length)))
               return false;
            break;
          case FieldType_double:
            if (!(EqualVal<double>::EqualScalar(addr1,addr2)))
               return false;
            break;
          case FieldType_doubleArray:
            if (!(EqualVal<double>::EqualArray(addr1,addr2,length)))
               return false;
            break;
          case FieldType_doubleVector:
            if (!(EqualVal<double>::EqualVector(addr1,addr2)))
               return false;
            break;
          case FieldType_uchar:
            if (!(EqualVal<unsigned char>::EqualScalar(addr1,addr2)))
               return false;
            break;
          case FieldType_ucharArray:
            if (!(EqualVal<unsigned char>::EqualArray(addr1,addr2,length)))
               return false;
            break;
          case FieldType_ucharVector:
            if (!(EqualVal<unsigned char>::EqualVector(addr1,addr2)))
               return false;
            break;
          case FieldType_string:
            if (!(EqualVal<vtkstd::string>::EqualScalar(addr1,addr2)))
               return false;
            break;
          case FieldType_stringVector:
            if (!(EqualVal<vtkstd::string>::EqualVector(addr1,addr2)))
               return false;
            break;
          case FieldType_colortable:
            if (!(EqualVal<vtkstd::string>::EqualScalar(addr1,addr2)))
               return false;
            break;
          case FieldType_opacity:
            if (!(EqualVal<double>::EqualScalar(addr1,addr2)))
               return false;
            break;
          case FieldType_linestyle:
            if (!(EqualVal<int>::EqualScalar(addr1,addr2)))
               return false;
            break;
          case FieldType_linewidth:
            if (!(EqualVal<int>::EqualScalar(addr1,addr2)))
               return false;
            break;
          case FieldType_variablename:
            if (!(EqualVal<vtkstd::string>::EqualScalar(addr1,addr2)))
               return false;
            break;
          case FieldType_color:
          case FieldType_att:
            if (!(((AttributeGroup*)addr1)->EqualTo((AttributeGroup*)addr2)))
               return false;
            break;
          case FieldType_attVector:
            {
                AttributeGroupVector &a1 =*(AttributeGroupVector*)addr1;
                AttributeGroupVector &a2 =*(AttributeGroupVector*)addr2;
                if (!(EqualVal<AttributeGroup*>::EqualVector(&a1, &a2)))
                    return false;
            }
            break;
          case FieldType_enum:
            if (!(EqualVal<int>::EqualScalar(addr1,addr2)))
               return false;
            break;
          case FieldType_scalemode:
            if (!(EqualVal<int>::EqualScalar(addr1,addr2)))
               return false;
            break;
          default:
            cerr << "UNKNOWN TYPE IN AttributeGroup::EqualTo\n";
            return false;
        }
    }
    return true;
}

// ****************************************************************************
// Method: AttributeGroup::TypeName
//
// Purpose:
//   Returns the name of the type.
//
// Returns:    The name of the type.
//
// Programmer: Brad Whitlock
// Creation:   Tue Oct 9 15:42:05 PST 2001
//
// Modifications:
//
// ****************************************************************************

const vtkstd::string
AttributeGroup::TypeName() const
{
    return "AttributeGroup";
}

// ****************************************************************************
// Method: AttributeGroup::WriteType
//
// Purpose:
//   Writes an attribute to the Connection.
//
// Arguments:
//   conn     : A reference to the Connection object to which the
//              attribute is being written.
//   typeCode : The type of the attribute. (See the top of this file)
//   address  : A pointer to attribute.
//   length   : The length of the list, if the attribute is a list.
//
// Note:
//   This is a LONG switch statement. If you add a new type, don't
//   forget to put in the break for the new case.
//
// Programmer: Brad Whitlock
// Creation:   Fri Aug 4 16:34:32 PST 2000
//
// Modifications:
//    Jeremy Meredith, Mon Feb 26 16:03:02 PST 2001
//    Added unsigned chars.
//
// ****************************************************************************
/*
void
AttributeGroup::WriteType(Connection &conn, AttributeGroup::typeInfo &info)
{
    int i;

    switch(info.typeCode)
    {
    case msgTypeChar:
        conn.WriteChar(*((char *)info.address));
        break;
    case msgTypeUnsignedChar:
        conn.WriteUnsignedChar(*((unsigned char *)info.address));
        break;
    case msgTypeInt:
        conn.WriteInt(*((int *)info.address));
        break;
    case msgTypeLong:
        conn.WriteLong(*((long *)info.address));
        break;
    case msgTypeFloat:
        conn.WriteFloat(*((float *)info.address));
        break;
    case msgTypeDouble:
        conn.WriteDouble(*((double *)info.address));
        break;
    case msgTypeString:
        { // new scope
          // Write a vtkstd::string to the connection
          vtkstd::string *sptr = (vtkstd::string *)(info.address);

          for(i = 0; i < sptr->size(); ++i)
              conn.WriteChar(sptr->at(i));
          conn.WriteChar(0);
        }
        break;
    case msgTypeAttributeGroup:
        { // new scope
           // Cast the address into another attributeGroup and write the
           // sub-AttributeGroup onto the connection.
           AttributeGroup *aptr = (AttributeGroup *)(info.address);
           aptr->Write(conn);
        }
        break;
    case msgTypeBool:
        // Write a bool as a character.
        if(*((bool *)info.address))
            conn.WriteChar(1);
        else
            conn.WriteChar(0);

        break;
    case msgTypeListChar:
        { // new scope
           char *cptr = (char *)(info.address);

           conn.WriteInt(info.length);
           for(i = 0; i < info.length; ++i, ++cptr)
              conn.WriteChar(*cptr);
        }
        break;
    case msgTypeListUnsignedChar:
        { // new scope
           unsigned char *uptr = (unsigned char *)(info.address);

           conn.WriteInt(info.length);
           for(i = 0; i < info.length; ++i, ++uptr)
              conn.WriteUnsignedChar(*uptr);
        }
        break;
    case msgTypeListInt:
        { // new scope
          int *iptr = (int *)(info.address);

          conn.WriteInt(info.length);
          for(i = 0; i < info.length; ++i, ++iptr)
              conn.WriteInt(*iptr);
        }
        break;
    case msgTypeListLong:
        { // new scope
          long *lptr = (long *)(info.address);

          conn.WriteInt(info.length);
          for(i = 0; i < info.length; ++i, ++lptr)
              conn.WriteLong(*lptr);
        }
        break;
    case msgTypeListFloat:
        { // new scope
          float *fptr = (float *)(info.address);

          conn.WriteInt(info.length);
          for(i = 0; i < info.length; ++i, ++fptr)
              conn.WriteFloat(*fptr);
        }
        break;
    case msgTypeListDouble:
        { // new scope
          double *dptr = (double *)(info.address);

          conn.WriteInt(info.length);
          for(i = 0; i < info.length; ++i, ++dptr)
              conn.WriteDouble(*dptr);
        }
        break;
    case msgTypeListString:
        { // new scope
          vtkstd::string *sptr = (vtkstd::string *)(info.address);
          int j;

          conn.WriteInt(info.length);
          for(i = 0; i < info.length; ++i)
          {
              for(j = 0; j < sptr[i].size(); ++j)
                  conn.WriteChar(sptr[i].at(j));
              conn.WriteChar(0);
          }
        }
        break;
    case msgTypeListAttributeGroup:
        { // new scope
          AttributeGroup **aptr = (AttributeGroup **)(info.address);

          conn.WriteInt(info.length);
          for(i = 0; i < info.length; ++i, ++aptr)
          {
              if((*aptr) != 0)
                  (*aptr)->Write(conn);
          }
        }
        break;
    case msgTypeListBool:
        { // new scope
           bool *bptr = (bool *)(info.address);

           conn.WriteInt(info.length);
           for(i = 0; i < info.length; ++i, ++bptr)
           {
               if(*bptr)
                   conn.WriteChar(1);
               else
                   conn.WriteChar(0);
           }
        }
        break;
    case msgTypeVectorBool:
        { // new scope
          boolVector *vb = (boolVector *)(info.address);
          boolVector::iterator bpos;

          conn.WriteInt(vb->size());
          for(bpos = vb->begin(); bpos != vb->end(); ++bpos)
          {
              if (*bpos)
                  conn.WriteChar(1);
              else
                  conn.WriteChar(0);
          }
        }
        break;
    case msgTypeVectorChar:
        { // new scope
          charVector *vc = (charVector *)(info.address);
          charVector::iterator cpos;

          conn.WriteInt(vc->size());
          for(cpos = vc->begin(); cpos != vc->end(); ++cpos)
              conn.WriteChar(*cpos);
        }
        break;
    case msgTypeVectorUnsignedChar:
        { // new scope
          unsignedCharVector *vc = (unsignedCharVector *)(info.address);
          unsignedCharVector::iterator cpos;

          conn.WriteInt(vc->size());
          for(cpos = vc->begin(); cpos != vc->end(); ++cpos)
              conn.WriteUnsignedChar(*cpos);
        }
        break;
    case msgTypeVectorInt:
        { // new scope
          intVector *vi = (intVector *)(info.address);
          intVector::iterator ipos;
          conn.WriteInt(vi->size());
          for(ipos = vi->begin(); ipos != vi->end(); ++ipos)
              conn.WriteInt(*ipos);
        }
        break;
    case msgTypeVectorLong:
        { // new scope
          longVector *vl = (longVector *)(info.address);
          longVector::iterator lpos;

          conn.WriteInt(vl->size());
          for(lpos = vl->begin(); lpos != vl->end(); ++lpos)
              conn.WriteLong(*lpos);
        }
        break;
    case msgTypeVectorFloat:
        { // new scope
          floatVector *vf = (floatVector *)(info.address);
          floatVector::iterator fpos;

          conn.WriteInt(vf->size());
          for(fpos = vf->begin(); fpos != vf->end(); ++fpos)
              conn.WriteFloat(*fpos);
        }
        break;
    case msgTypeVectorDouble:
        { // new scope
          doubleVector *vd = (doubleVector *)(info.address);
          doubleVector::iterator dpos;

          conn.WriteInt(vd->size());
          for(dpos = vd->begin(); dpos != vd->end(); ++dpos)
              conn.WriteDouble(*dpos);
        }
        break;
    case msgTypeVectorString:
        { // new scope
          stringVector *vs = (stringVector *)(info.address);
          stringVector::iterator spos;

          conn.WriteInt(vs->size());
          // Write the strings out as C strings.
          for(spos = vs->begin(); spos != vs->end(); ++spos)
          {
              for(i = 0; i < spos->size(); ++i)
                  conn.WriteChar(spos->at(i));
              conn.WriteChar(0);
          }
        }
        break;
    case msgTypeVectorAttributeGroup:
        { // new scope
          AttributeGroupVector *va = (AttributeGroupVector *)(info.address);
          AttributeGroupVector::iterator apos;

          conn.WriteInt(va->size());
          // Write out the AttributeGroups
          for(apos = va->begin(); apos != va->end(); ++apos)
          {
              (*apos)->Write(conn);
          }
        }
    case msgTypeNone:
    default:
        ; // nothing.
    }
}
*/
// ****************************************************************************
// Method: AttributeGroup::ReadType
//
// Purpose:
//   Reads an attribute from the connection.
//
// Arguments:
//   conn     : The Connection obect from which the attribute is read.
//   typeCode : The type of the attribute to be read.
//   address  : The address into which the attribute is stored.
//   length   : If the attribute is a list, length is the expected
//              length.
//
// Programmer: Brad Whitlock
// Creation:   Fri Aug 4 16:49:51 PST 2000
//
// Modifications:
//   Brad Whitlock, Mon Aug 28 10:49:19 PDT 2000
//   I added code to prevent the vectors from trying to reserve zero
//   elements. That was causing crashes.
//
//   Jeremy Meredith, Mon Feb 26 16:03:02 PST 2001
//   Added unsigned chars.
//
//   Brad Whitlock, Fri Feb 1 13:34:22 PST 2002
//   Changed code for AttributeGroupVectors so it calls a virtual destructor
//   instead of the base class's destructor. This fixes a memory leak.
//
// ****************************************************************************
/*
void
AttributeGroup::ReadType(Connection &conn, int attrId, AttributeGroup::typeInfo &info)
{
    int i, vecLen;

    switch(info.typeCode)
    {
    case msgTypeChar:
        conn.ReadChar((unsigned char *)(info.address));
        break;
    case msgTypeUnsignedChar:
        conn.ReadUnsignedChar((unsigned char *)(info.address));
        break;
    case msgTypeInt:
        conn.ReadInt((int *)(info.address));
        break;
    case msgTypeLong:
        conn.ReadLong((long *)(info.address));
        break;
    case msgTypeFloat:
        conn.ReadFloat((float *)(info.address));
        break;
    case msgTypeDouble:
        conn.ReadDouble((double *)(info.address));
        break;
    case msgTypeString:
        { // new scope
          unsigned char c;
          vtkstd::string *sptr = (vtkstd::string *)(info.address);
          sptr->erase();

          // Read characters until there is a null-terminator.
          do
          {
              conn.ReadChar(&c);
              if(c != '\0')
                  *sptr += char(c);
          }
          while(c != '\0');
        }
        break;
    case msgTypeAttributeGroup:
        { // new scope
           // Recursively read the AttributeGroup
          AttributeGroup *aptr = (AttributeGroup *)(info.address);
          aptr->Read(conn);
        }
        break;
    case msgTypeBool:
        { // new scope
          unsigned char c;
          bool *b = (bool *)(info.address);
          conn.ReadChar(&c);

          *b = (c == 1);
        }
        break;
    case msgTypeListChar:
        { // new scope
          unsigned char *cptr = (unsigned char *)(info.address);

          conn.ReadInt(&(info.length));
          for(i = 0; i < info.length; ++i, ++cptr)
              conn.ReadChar(cptr);
        }
        break;
    case msgTypeListUnsignedChar:
        { // new scope
          unsigned char *uptr = (unsigned char *)(info.address);

          conn.ReadInt(&(info.length));
          for(i = 0; i < info.length; ++i, ++uptr)
              conn.ReadUnsignedChar(uptr);
        }
        break;
    case msgTypeListInt:
        { // new scope
          int *iptr = (int *)(info.address);

          conn.ReadInt(&(info.length));
          for(i = 0; i < info.length; ++i, ++iptr)
              conn.ReadInt(iptr);
        }
        break;
    case msgTypeListLong:
        { // new scope
          long *lptr = (long *)(info.address);

          conn.ReadInt(&(info.length));
          for(i = 0; i < info.length; ++i, ++lptr)
              conn.ReadLong(lptr);
        }
        break;
    case msgTypeListFloat:
        { // new scope
          float *fptr = (float *)(info.address);

          conn.ReadInt(&(info.length));
          for(i = 0; i < info.length; ++i, ++fptr)
              conn.ReadFloat(fptr);
        }
        break;
    case msgTypeListDouble:
        { // new scope
          double *dptr = (double *)(info.address);

          conn.ReadInt(&(info.length));
          for(i = 0; i < info.length; ++i, ++dptr)
              conn.ReadDouble(dptr);
        }
        break;
    case msgTypeListString:
        { // new scope
          vtkstd::string *sptr = (vtkstd::string *)(info.address);

          conn.ReadInt(&(info.length));
          for(i = 0; i < info.length; ++i)
          {
              unsigned char c = 'a';
              sptr[i].erase();

              // Read characters until there is a null-terminator.
              do
              {
                  conn.ReadChar(&c);
                  if(c != '\0')
                      sptr[i] += char(c);
              }
              while(c != '\0');
          }
        }
        break;
    case msgTypeListAttributeGroup:
        { // new scope
          AttributeGroup **aptr = (AttributeGroup **)(info.address);

          // Recursively read the AttributeGroups in the list.
          conn.ReadInt(&(info.length));
          for(i = 0; i < info.length; ++i)
          {
              if((*aptr) != 0)
                  (*aptr)->Read(conn);
          }
        }
        break;
    case msgTypeListBool:
        { // new scope
          bool *bptr = (bool *)(info.address);

          conn.ReadInt(&(info.length));
          for(i = 0; i < info.length; ++i, ++bptr)
          {
              unsigned char c;
              conn.ReadChar(&c);

              *bptr = (c == 1);
          }
        }
        break;
    case msgTypeVectorBool:
        { // new scope
          boolVector *vb = (boolVector *)(info.address);
          vb->clear();

          // Read the length of the vector and reserve that many items.
          conn.ReadInt(&vecLen);
          if(vecLen > 0)
              vb->reserve(vecLen);

          // Read the elements
          for(i = vecLen; i > 0; --i)
          {
              unsigned char c;
              conn.ReadChar(&c);
              vb->push_back((c==1));
          }
        }
        break;
    case msgTypeVectorChar:
        { // new scope
          unsigned char c;
          charVector *vc = (charVector *)(info.address);
          vc->clear();

          // Read the length of the vector and reserve that many items.
          conn.ReadInt(&vecLen);
          if(vecLen > 0)
              vc->reserve(vecLen);

          // Read the elements
          for(i = vecLen; i > 0; --i)
          {
              conn.ReadChar(&c);
              vc->push_back((char)c);
          }
        }
        break;
    case msgTypeVectorUnsignedChar:
        { // new scope
          unsigned char c;
          unsignedCharVector *vc = (unsignedCharVector *)(info.address);
          vc->clear();

          // Read the length of the vector and reserve that many items.
          conn.ReadInt(&vecLen);
          if(vecLen > 0)
              vc->reserve(vecLen);

          // Read the elements
          for(i = vecLen; i > 0; --i)
          {
              conn.ReadUnsignedChar(&c);
              vc->push_back((char)c);
          }
        }
        break;
    case msgTypeVectorInt:
        { // new scope
          int ival;
          intVector *vi = (intVector *)(info.address);
          vi->clear();

          // Read the length of the vector and reserve that many items.
          conn.ReadInt(&vecLen);
          if(vecLen > 0)
              vi->reserve(vecLen);

          // Read the elements
          for(i = vecLen; i > 0; --i)
          {
              conn.ReadInt(&ival);
              vi->push_back(ival);
          }
        }
        break;
    case msgTypeVectorLong:
        { // new scope
          long lval;
          longVector *vl = (longVector *)(info.address);
          vl->clear();

          // Read the length of the vector and reserve that many items.
          conn.ReadInt(&vecLen);
          if(vecLen > 0)
              vl->reserve(vecLen);

          // Read the elements
          for(i = vecLen; i > 0; --i)
          {
              conn.ReadLong(&lval);
              vl->push_back(lval);
          }
        }
        break;
    case msgTypeVectorFloat:
        { // new scope
          float fval;
          floatVector *vf = (floatVector *)(info.address);
          vf->clear();

          // Read the length of the vector and reserve that many items.
          conn.ReadInt(&vecLen);
          if(vecLen > 0)
              vf->reserve(vecLen);

          // Read the elements
          for(i = vecLen; i > 0; --i)
          {
              conn.ReadFloat(&fval);
              vf->push_back(fval);
          }
        }
        break;
    case msgTypeVectorDouble:
        { // new scope
          double dval;
          doubleVector *vd = (doubleVector *)(info.address);
          vd->clear();

          // Read the length of the vector and reserve that many items.
          conn.ReadInt(&vecLen);
          if(vecLen > 0)
              vd->reserve(vecLen);

          // Read the elements
          for(i = vecLen; i > 0; --i)
          {
              conn.ReadDouble(&dval);
              vd->push_back(dval);
          }
        }
        break;
    case msgTypeVectorString:
        { // new scope
          stringVector *vs = (stringVector *)(info.address);
          vs->clear();

          // Read the length of the vector and reserve that many items.
          conn.ReadInt(&vecLen);
          if(vecLen > 0)
              vs->reserve(vecLen);

          // Read the elements
          for(i = vecLen; i > 0; --i)
          {
              unsigned char c;
              vtkstd::string   str;

              // Read characters until there is a null-terminator.
              do
              {
                  conn.ReadChar(&c);
                  if(c != '\0')
                      str += char(c);
              }
              while(c != '\0');

              // Add the string to the list.
              vs->push_back(str);
          }
        }
        break;
    case msgTypeVectorAttributeGroup:
        { // new scope
          AttributeGroupVector *va = (AttributeGroupVector *)(info.address);
          AttributeGroupVector::iterator apos;

          // Destroy the sub-AttributeGroups
          for(apos = va->begin(); apos != va->end(); ++apos)
              delete (*apos);

          // Read the length of the vector and reserve that many items.
          conn.ReadInt(&vecLen);
          va->clear();
          if(vecLen > 0)
              va->reserve(vecLen);

          for(i = 0; i < vecLen; ++i)
          {
              AttributeGroup *new_ag;
              new_ag = CreateSubAttributeGroup(attrId);
              new_ag->Read(conn);
              va->push_back(new_ag);
          }
        }
        break;
    case msgTypeNone:
    default:
        ; // nothing.
    }
}
*/
// ****************************************************************************
// Method: AttributeGroup::CreateSubAttributeGroup
//
// Purpose:
//   This method returns a pointer to a new AttributeGroup object.
//   Subclasses that contain an AttributeGroupVector must override
//   this method. This method is used when allocating storage for an
//   object that is the result of reading an AttributeGroupVector.
//
// Arguments:
//   attrId : This argument tells which kind of AttributeGroup to
//            instantiate and return.
//
// Returns:
//   A pointer to a new AttributeGroup object. The default is 0.
//
// Note:
//
// Programmer: Brad Whitlock
// Creation:   Tue Aug 8 13:35:52 PST 2000
//
// Modifications:
//
// ****************************************************************************

AttributeGroup *
AttributeGroup::CreateSubAttributeGroup(int)
{
    return 0;
}

// ****************************************************************************
// Method: AttributeGroup::Select
//
// Purpose:
//   Selects an attribute so it can be transmitted. It also associates
//   a real memory address with the attribute in the type map. This is
//   required in order to write or read the attribute.
//
// Arguments:
//   index   : The index of the attribute being selected.
//   address : The address of the attribute being selected.
//   length  : The number of elements in the attribute if it is a list.
//             The default value is zero.
//
// Programmer: Brad Whitlock
// Creation:   Fri Aug 4 16:56:22 PST 2000
//
// Modifications:
//
// ****************************************************************************

void
AttributeGroup::Select(int index, void *address, int length)
{
    if(index < static_cast<int>(typeMap.size()))
    {
        typeMap[index].address = address;
        typeMap[index].selected = true;
        typeMap[index].length = length;
    }
}

// ****************************************************************************
// Method: AttributeGroup::SelectField
//
// Purpose:
//   Lets the user select a field.
//
// Arguments:
//   index : The field to select.
//
// Programmer: Brad Whitlock
// Creation:   Tue May 3 11:11:08 PDT 2005
//
// Modifications:
//
// ****************************************************************************

void
AttributeGroup::SelectField(int index)
{
    if(index >= 0 && index < static_cast<int>(typeMap.size()))
    {
        if(typeMap[index].address != 0)
            typeMap[index].selected = true;
    }
}

// ****************************************************************************
// Method: AttributeGroup::SelectFields
//
// Purpose:
//   Lets the user select multiple fields.
//
// Arguments:
//   indices : The indices of the fields to select.
//
// Programmer: Brad Whitlock
// Creation:   Tue May 3 11:11:32 PDT 2005
//
// Modifications:
//
// ****************************************************************************

void
AttributeGroup::SelectFields(const vtkstd::vector<int> &indices)
{
    // Select and unselect to make sure that the addresses are all okay.
    SelectAll();
    if(indices.size() > 0)
    {
        UnSelectAll();

        for(unsigned int i = 0; i < indices.size(); ++i)
        {
            int index = indices[i];
            if(index >= 0 && index < static_cast<int>(typeMap.size()))
                typeMap[index].selected = true;
        }
    }
}

// ****************************************************************************
// Method: AttributeGroup::UnSelectAll
//
// Purpose:
//   Unselects all the attributes in the typemap. This indicates that
//   they have not changed.
//
// Programmer: Brad Whitlock
// Creation:   Tue Aug 8 13:48:17 PST 2000
//
// Modifications:
//
// ****************************************************************************

void
AttributeGroup::UnSelectAll()
{
    // Unselect all the components in the typeMap.
    typeInfoVector::iterator pos;
    for(pos = typeMap.begin(); pos != typeMap.end(); ++pos)
    {
        pos->selected = false;
    }
}

// ****************************************************************************
// Method: AttributeGroup::CreateNode
//
// Purpose:
//   Creates a DataNode representation of the AttributeGroup and
//   adds it to the node that is passed in. Unless a subclass, overrides
//   this method, no DataNode is created.
//
// Programmer: Brad Whitlock
// Creation:   Thu Sep 28 12:40:58 PDT 2000
//
// Modifications:
//   Brad Whitlock, Tue May 20 08:55:07 PDT 2003
//   I added the bool argument and made it return a bool indicating whether
//   or not anything was added to the node.
//
//   Brad Whitlock, Wed Dec 17 11:53:32 PDT 2003
//   I added another bool argument that will be used for making the complete
//   object save out regardless of whether or not it is equal to the
//   defaults.
//
// ****************************************************************************

bool
AttributeGroup::CreateNode(DataNode *, bool, bool)
{
    return false;
}

// ****************************************************************************
// Method: AttributeGroup::SetFromNode
//
// Purpose:
//   Reads the values in the DataNode and sets attributes in the
//   AttributeGroup from those values. Unless this method is overridden
//   in subclasses, it does nothing.
//
// Programmer: Brad Whitlock
// Creation:   Thu Sep 28 12:40:58 PDT 2000
//
// Modifications:
//
// ****************************************************************************

void
AttributeGroup::SetFromNode(DataNode *)
{
    // nothing.
}

// ****************************************************************************
// Method: AttributeGroup::ProcessOldVersions
//
// Purpose:
//   Processes old versions of the DataNode so that it is up to date when
//   we call SetFromNode.
//
// Arguments:
//   node          : The data node of the parent.
//   configVersion : The version of the config information in the DataNode.
//
// Programmer: Brad Whitlock
// Creation:   Fri Mar 21 09:50:31 PDT 2003
//
// Modifications:
//
// ****************************************************************************

void
AttributeGroup::ProcessOldVersions(DataNode *, const char *)
{
    // nothing
}

// ****************************************************************************
// Method: AttributeGroup::VersionLessThan
//
// Purpose:
//   Compares version strings to determine if the configVersion is less than
//   the current version.
//
// Arguments:
//   configVersion : The version of the config file.
//   version       : The version that we're comparing to.
//
// Returns:    true if the configVersion is less than the version.
//
// Programmer: Brad Whitlock
// Creation:   Fri Mar 21 09:53:56 PDT 2003
//
// Modifications:
//   Brad Whitlock, Wed Mar 8 10:24:43 PDT 2006
//   I improved the code so it takes beta versions into account. This way,
//   a version of 1.5.2b will be less than 1.5.2.
//
// ****************************************************************************

#define VERSION_3_TO_NUM(m,n,p,b)    ((m)+(n)/100.0+(p)/10000.0+(b)/100000.0)

bool
AttributeGroup::VersionLessThan(const char *configVersion, const char *version)
{
    int versions[2][3] = {{0,0,0}, {0,0,0}};
    int betas[2] = {0, 0};
    const char *versionStrings[] = {configVersion, version};
    char storage[30];

    if(configVersion == 0 && version != 0)
        return true;
    if(configVersion != 0 && version == 0)
        return false;
    if(configVersion == 0 && version == 0)
        return false;

    for(int i = 0; i < 2; ++i)
    {
        // Go to the first space and copy the version number string into
        // the buffer.
        char *buf = storage;
        strncpy(buf, versionStrings[i], 30);

        // Indicate whether the version number has a beta in it.
        int len = static_cast<int>(strlen(buf));
        if(len > 0)
            betas[i] = (buf[len-1] == 'b') ? 0 : 1;
        else
            betas[i] = 1;

        // Use strtok to get all of the version numbers.  Note that atoi()
        // returns 0 if given a NULL string, which is what we want.
        char *p = strtok(buf, ".");
        if(p)
        {
            versions[i][0] = atoi(p);
            p = strtok(NULL, ".");
            if(p)
            {
                versions[i][1] = atoi(p);
                p = strtok(NULL, ".");
                if(p)
                    versions[i][2] = atoi(p);
            }
        }
    }

    return VERSION_3_TO_NUM(versions[0][0], versions[0][1], versions[0][2], betas[0]) <
           VERSION_3_TO_NUM(versions[1][0], versions[1][1], versions[1][2], betas[1]);
}

// ****************************************************************************
// Method: AttributeGroup::Write
//
// Purpose:
//   Writes the selected attributes of the AttributeGroup onto a
//   Connection object.
//
// Arguments:
//   conn : The connection object to which we're writing.
//
// Note:
//   If no attributes are selected, SelectAll is called so all the
//   attributes are written.
//
// Programmer: Brad Whitlock
// Creation:   Fri Aug 4 17:00:46 PST 2000
//
// Modifications:
//    Jeremy Meredith, Mon Feb 26 16:05:36 PST 2001
//    Made length write using unsigned chars.
//
// ****************************************************************************
/*
void
AttributeGroup::Write(Connection &conn)
{
    // If there are no selected components. Select them all.
    if(NumAttributesSelected() == 0)
        SelectAll();

    // Write the number of selected attributes
    if(typeMap.size() < 256)
        conn.WriteUnsignedChar((unsigned char)NumAttributesSelected());
    else
        conn.WriteInt(NumAttributesSelected());

    // Write the selected attributes.
    for(int i = 0; i < typeMap.size(); ++i)
    {
        if(typeMap[i].selected)
        {
            // Write the attribute's index
            if(typeMap.size() < 256)
                conn.WriteUnsignedChar((unsigned char)i);
            else
                conn.WriteInt(i);

            // Write the attribute's data
            WriteType(conn, typeMap[i]);
        }
    }
}
*/
// ****************************************************************************
// Method: AttributeGroup::Read
//
// Purpose:
//   Reads the AttributeGroup from a Connection object. Once the read
//   has happened, only attributes that have changed are selected.
//
// Arguments:
//   conn : The Connection object from which we're reading.
//
// Programmer: Brad Whitlock
// Creation:   Fri Aug 4 17:02:55 PST 2000
//
// Modifications:
//    Jeremy Meredith, Mon Feb 26 16:05:36 PST 2001
//    Made length read using unsigned chars.
//
// ****************************************************************************
/*
void
AttributeGroup::Read(Connection &conn)
{
    unsigned char cval;
    int nComponents, i;

    // Select all the components to get their destination addresses.
    SelectAll();
    // Unselect everything now that we have the addresses.
    UnSelectAll();

    // Read the number of components from the connection.
    if(typeMap.size() < 256)
    {
        conn.ReadUnsignedChar(&cval);
        nComponents = (int)cval;
    }
    else
        conn.ReadInt(&nComponents);

    // Read all the components that are in the message.
    for(i = 0; i < nComponents; ++i)
    {
        int attrIndex;

        // Read the Id of the next attribute. The size can vary
        // depending on the number of attributes.
        if(typeMap.size() < 256)
        {
            conn.ReadUnsignedChar(&cval);
            attrIndex = (int)cval;
        }
        else
            conn.ReadInt(&attrIndex);

        // Read the attribute if the attrIndex is valid. Indicate that
        // it is selected.
        if(attrIndex < typeMap.size())
        {
            ReadType(conn, i, typeMap[attrIndex]);
            typeMap[attrIndex].selected = true;
        }
    }
}
*/
// ****************************************************************************
// Method: AttributeGroup::NumAttributesSelected
//
// Purpose:
//   Returns the number of attributes that are selected.
//
// Returns:
//   The number of attributes that are selected.
//
// Programmer: Brad Whitlock
// Creation:   Fri Aug 4 17:04:23 PST 2000
//
// Modifications:
//
// ****************************************************************************

int
AttributeGroup::NumAttributesSelected() const
{
    // See if any fields are selected. If none are selected
    // then we will assume that we want to write all of them since
    // it makes no sense to write none.
    int selectCount = 0;
    typeInfoVector::const_iterator pos;
    for(pos = typeMap.begin(); pos != typeMap.end(); ++pos)
    {
        if(pos->selected)
            ++selectCount;
    }

    return selectCount;
}

// ****************************************************************************
// Method: AttributeGroup::CalculateMessageSize
//
// Purpose:
//   Calculates the predicted messages size in bytes for the
//   attributes which are currently selected.
//
// Arguments:
//   conn : The connection to which the AttributeGroup will be
//          written. This is relevant because the connection knows
//          how the sizes of the destination machine's built-in types.
//
// Returns:
//   The number of bytes in the message that will be written.
//
// Programmer: Brad Whitlock
// Creation:   Fri Aug 4 17:05:10 PST 2000
//
// Modifications:
//   Brad Whitlock, Mon Aug 28 18:32:00 PST 2000
//   I added a forgotten case for the AttributeGroupVector.
//
// ****************************************************************************
/*
int
AttributeGroup::CalculateMessageSize(Connection &conn)
{
    int i, messageSize = 0;

    // If there are no attributes selected, select them all.
    if(NumAttributesSelected() == 0)
        SelectAll();

    // Add the size of one int|char for the number of attributes.
    int attrSize = (typeMap.size() < 256) ? conn.CharSize(conn.DEST) :
        conn.IntSize(conn.DEST);
    messageSize += attrSize;

    // Add an int|char for each of the selected attributes.
    messageSize += (NumAttributesSelected() * attrSize);

    // Add the sizes of the selected components
    typeInfoVector::iterator pos;
    for(pos = typeMap.begin(); pos != typeMap.end(); ++pos)
    {
        if(pos->selected)
        {
            switch(pos->typeCode)
            {
            case msgTypeChar:
                messageSize += conn.CharSize(conn.DEST);
                break;
            case msgTypeUnsignedChar:
                messageSize += conn.CharSize(conn.DEST);
                break;
            case msgTypeInt:
                messageSize += conn.IntSize(conn.DEST);
                break;
            case msgTypeLong:
                messageSize += conn.LongSize(conn.DEST);
                break;
            case msgTypeFloat:
                messageSize += conn.FloatSize(conn.DEST);
                break;
            case msgTypeDouble:
                messageSize += conn.DoubleSize(conn.DEST);
                break;
            case msgTypeString:
            { // new scope
                vtkstd::string *sptr = (vtkstd::string *)(pos->address);
                messageSize += (conn.CharSize(conn.DEST) * (sptr->size() + 1));
            }
                break;
            case msgTypeAttributeGroup:
            { // new scope
                // Recursively figure out the message size.
                AttributeGroup *aptr = (AttributeGroup *)pos->address;
                messageSize += aptr->CalculateMessageSize(conn);
            }
                break;
            case msgTypeBool:
                messageSize += conn.CharSize(conn.DEST);
                break;
            case msgTypeListChar:
            case msgTypeListUnsignedChar:
            case msgTypeListBool:
                messageSize += conn.IntSize(conn.DEST);
                messageSize += (pos->length * conn.CharSize(conn.DEST));
                break;
            case msgTypeListInt:
                messageSize += conn.IntSize(conn.DEST);
                messageSize += (pos->length * conn.IntSize(conn.DEST));
                break;
            case msgTypeListLong:
                messageSize += conn.IntSize(conn.DEST);
                messageSize += (pos->length * conn.LongSize(conn.DEST));
                break;
            case msgTypeListFloat:
                messageSize += conn.IntSize(conn.DEST);
                messageSize += (pos->length * conn.FloatSize(conn.DEST));
                break;
            case msgTypeListDouble:
                messageSize += conn.IntSize(conn.DEST);
                messageSize += (pos->length * conn.DoubleSize(conn.DEST));
                break;
            case msgTypeListString:
            { // new scope
                vtkstd::string *sptr = (vtkstd::string *)(pos->address);

                messageSize += conn.IntSize(conn.DEST);
                for(i = 0; i < pos->length; ++i, ++sptr)
                {
                    messageSize += (conn.CharSize(conn.DEST) *
                        (sptr->size() + 1));
                }
            }
                break;
            case msgTypeListAttributeGroup:
            { // new scope
                AttributeGroup **aptr = (AttributeGroup **)pos->address;

                // Recursively figure out the message size.
                messageSize += conn.IntSize(conn.DEST);
                for(i = 0; i < pos->length; ++i, ++aptr)
                {
                    if((*aptr) != 0)
                       messageSize += (*aptr)->CalculateMessageSize(conn);
                }
            }
                break;
            case msgTypeVectorBool:
            { // new scope
                boolVector *vb = (boolVector *)(pos->address);
                messageSize += conn.IntSize(conn.DEST);
                messageSize += (conn.CharSize(conn.DEST) * vb->size());
            }
                break;
            case msgTypeVectorChar:
            { // new scope
                charVector *vc = (charVector *)(pos->address);
                messageSize += conn.IntSize(conn.DEST);
                messageSize += (conn.CharSize(conn.DEST) * vc->size());
            }
                break;
            case msgTypeVectorUnsignedChar:
            { // new scope
                unsignedCharVector *vc = (unsignedCharVector *)(pos->address);
                messageSize += conn.IntSize(conn.DEST);
                messageSize += (conn.CharSize(conn.DEST) * vc->size());
            }
                break;
            case msgTypeVectorInt:
            { // new scope
                intVector *vi = (intVector *)(pos->address);
                messageSize += conn.IntSize(conn.DEST);
                messageSize += (conn.IntSize(conn.DEST) * vi->size());
            }
                break;
            case msgTypeVectorLong:
            { // new scope
                longVector *vl = (longVector *)(pos->address);
                messageSize += conn.IntSize(conn.DEST);
                messageSize += (conn.LongSize(conn.DEST) * vl->size());
            }
                break;
            case msgTypeVectorFloat:
            { // new scope
                floatVector *vf = (floatVector *)(pos->address);
                messageSize += conn.IntSize(conn.DEST);
                messageSize += (conn.FloatSize(conn.DEST) * vf->size());
            }
                break;
            case msgTypeVectorDouble:
            { // new scope
                doubleVector *vd = (doubleVector *)(pos->address);
                messageSize += conn.IntSize(conn.DEST);
                messageSize += (conn.DoubleSize(conn.DEST) * vd->size());
            }
                break;
            case msgTypeVectorString:
            { // new scope
                stringVector *vs = (stringVector *)(pos->address);
                stringVector::iterator spos;

                messageSize += conn.IntSize(conn.DEST);
                for(spos = vs->begin(); spos != vs->end(); ++spos)
                    messageSize += (conn.CharSize(conn.DEST) * (spos->size() + 1));
            }
                break;
            case msgTypeVectorAttributeGroup:
            { // new scope
                AttributeGroupVector *va = (AttributeGroupVector *)(pos->address);
                AttributeGroupVector::iterator apos;

                // Recursively figure out the message size.
                messageSize += conn.IntSize(conn.DEST);
                for(apos = va->begin(); apos != va->end(); ++apos)
                {
                    if((*apos) != 0)
                       messageSize += (*apos)->CalculateMessageSize(conn);
                }
            }
                break;
            case msgTypeNone:
            default:
                ; // nothing.
            }
        }
    } // end for

    return messageSize;
}
*/
//
// These methods are used to declare the Message's component types.
//
void AttributeGroup::DeclareChar()
{
    typeMap.push_back(msgTypeChar);
}

void AttributeGroup::DeclareUnsignedChar()
{
    typeMap.push_back(msgTypeUnsignedChar);
}

void AttributeGroup::DeclareInt()
{
    typeMap.push_back(msgTypeInt);
}

void
AttributeGroup::DeclareLong()
{
    typeMap.push_back(msgTypeLong);
}

void
AttributeGroup::DeclareFloat()
{
    typeMap.push_back(msgTypeFloat);
}

void
AttributeGroup::DeclareDouble()
{
    typeMap.push_back(msgTypeDouble);
}

void
AttributeGroup::DeclareString()
{
    typeMap.push_back(msgTypeString);
}

void
AttributeGroup::DeclareAttributeGroup()
{
    typeMap.push_back(msgTypeAttributeGroup);
}

void
AttributeGroup::DeclareBool()
{
    typeMap.push_back(msgTypeBool);
}

void
AttributeGroup::DeclareListChar()
{
    typeMap.push_back(msgTypeListChar);
}

void
AttributeGroup::DeclareListUnsignedChar()
{
    typeMap.push_back(msgTypeListUnsignedChar);
}

void
AttributeGroup::DeclareListInt()
{
    typeMap.push_back(msgTypeListInt);
}

void
AttributeGroup::DeclareListLong()
{
    typeMap.push_back(msgTypeListLong);
}

void
AttributeGroup::DeclareListFloat()
{
    typeMap.push_back(msgTypeListFloat);
}

void
AttributeGroup::DeclareListDouble()
{
    typeMap.push_back(msgTypeListDouble);
}

void
AttributeGroup::DeclareListString()
{
    typeMap.push_back(msgTypeListString);
}

void
AttributeGroup::DeclareListAttributeGroup()
{
    typeMap.push_back(msgTypeListAttributeGroup);
}

void
AttributeGroup::DeclareListBool()
{
    typeMap.push_back(msgTypeListBool);
}

void
AttributeGroup::DeclareVectorBool()
{
    typeMap.push_back(msgTypeVectorBool);
}

void
AttributeGroup::DeclareVectorChar()
{
    typeMap.push_back(msgTypeVectorChar);
}

void
AttributeGroup::DeclareVectorUnsignedChar()
{
    typeMap.push_back(msgTypeVectorUnsignedChar);
}

void
AttributeGroup::DeclareVectorInt()
{
    typeMap.push_back(msgTypeVectorInt);
}

void
AttributeGroup::DeclareVectorLong()
{
    typeMap.push_back(msgTypeVectorLong);
}

void
AttributeGroup::DeclareVectorFloat()
{
    typeMap.push_back(msgTypeVectorFloat);
}

void
AttributeGroup::DeclareVectorDouble()
{
    typeMap.push_back(msgTypeVectorDouble);
}

void
AttributeGroup::DeclareVectorAttributeGroup()
{
    typeMap.push_back(msgTypeVectorAttributeGroup);
}

void
AttributeGroup::DeclareVectorString()
{
    typeMap.push_back(msgTypeVectorString);
}

// ****************************************************************************
// Method: AttributeGroup::CreateTypeMap
//
// Purpose:
//   This is a convenience function that takes a format string and
//   adds the appropriate types to the type map.
//
// Arguments:
//   formatString : A string where each character represents the type
//                  of an attribute in the AttributeGroup.
//
// Programmer: Brad Whitlock
// Creation:   Fri Aug 4 17:15:29 PST 2000
//
// Modifications:
//   Brad Whitlock, Tue Aug 29 10:30:02 PDT 2000
//   I changed it so an empty or NULL format string is allowable.
//
//   Hank Childs, Fri Jan 28 15:36:03 PST 2005
//   Use exception macros.
//
// ****************************************************************************

void
AttributeGroup::CreateTypeMap(const char *formatString)
{
    // Clear the typeMap.
    typeMap.clear();

    // If the format string is NULL, return.
    if(formatString == NULL)
        return;

    // If the formatString was empty, get out of here.
    int nDeclares = static_cast<int>(strlen(formatString));
    if(nDeclares < 1)
        return;

    // Go through the format string and add the types.
    typeMap.reserve(nDeclares);
    for(int i = 0; i < nDeclares; ++i)
    {
        char baseFormat = formatString[i];

        // If the next character is a '*' then it is a vector.
        bool isVector = (i < nDeclares - 1) ? (formatString[i + 1] == '*') :
                        false;

        if(isVector)
        {
            // Advance one character
            ++i;

            // Declare vectors
            switch(baseFormat)
            {
            case 'b':
                DeclareVectorBool();
                break;
            case 'c':
                DeclareVectorChar();
                break;
            case 'u':
                DeclareVectorUnsignedChar();
                break;
            case 'i':
                DeclareVectorInt();
                break;
            case 'l':
                DeclareVectorLong();
                break;
            case 'f':
                DeclareVectorFloat();
                break;
            case 'd':
                DeclareVectorDouble();
                break;
            case 's':
                DeclareVectorString();
                break;
            case 'a':
                DeclareVectorAttributeGroup();
                break;
            default:
                EXCEPTION0(BadDeclareFormatString);
            }
        }
        else
        {
            // Declare simple types, fixed length, attributegroups
            switch(baseFormat)
            {
            case 'c':
                DeclareChar();
                break;
            case 'u':
                DeclareUnsignedChar();
                break;
            case 'i':
                DeclareInt();
                break;
            case 'l':
                DeclareLong();
                break;
            case 'f':
                DeclareFloat();
                break;
            case 'd':
                DeclareDouble();
                break;
            case 's':
                DeclareString();
                break;
            case 'a':
                DeclareAttributeGroup();
                break;
            case 'b':
                DeclareBool();
                break;
            case 'C':
                DeclareListChar();
                break;
            case 'U':
                DeclareListUnsignedChar();
                break;
            case 'I':
                DeclareListInt();
                break;
            case 'L':
                DeclareListLong();
                break;
            case 'F':
                DeclareListFloat();
                break;
            case 'D':
                DeclareListDouble();
                break;
            case 'S':
                DeclareListString();
                break;
            case 'A':
                DeclareListAttributeGroup();
                break;
            case 'B':
                DeclareListBool();
                break;
            default:
                EXCEPTION0(BadDeclareFormatString);
            }
        }
    }
}

void
AttributeGroup::SetGuido(int _guido)
{
    guido = _guido;
}

int
AttributeGroup::GetGuido()
{
    return guido;
}

AttributeGroup::typeInfo::typeInfo()
{
    typeCode = msgTypeNone;
    selected = false;
    address = NULL;
    length = 0;
}

AttributeGroup::typeInfo::typeInfo(const AttributeGroup::typeInfo &obj)
{
    typeCode = obj.typeCode;
    selected = obj.selected;
    address = obj.address;
    length = obj.length;
}

AttributeGroup::typeInfo::typeInfo(unsigned char tcode)
{
    typeCode = tcode;
    selected = false;
    address = NULL;
    length = 0;
}

AttributeGroup::typeInfo::~typeInfo()
{
}

void
AttributeGroup::typeInfo::operator =(const AttributeGroup::typeInfo &obj)
{
    typeCode = obj.typeCode;
    selected = obj.selected;
    address = obj.address;
    length = obj.length;
}

vtkstd::string
AttributeGroup::GetFieldName(int /*index*/) const
{
    return "<UNKNOWN name>";
}

AttributeGroup::FieldType
AttributeGroup::GetFieldType(int /*index*/) const
{
    return FieldType_unknown;
}

vtkstd::string
AttributeGroup::GetFieldTypeName(int /*index*/) const
{
    return "<UNKNOWN type>";
}

bool
AttributeGroup::FieldsEqual(int /*index*/, const AttributeGroup * /*rhs*/) const
{
    return false;
}

static int indentLevel = 0;
// ****************************************************************************
// Operator: <<
//
// Purpose: stream out a reasonably well formatted attribute group
//
// Programmer: Mark C. Miller
// Creation:   Tue Nov 26 08:57:53 PST 2002
//
// Modifications:
//   Brad Whitlock, Wed Dec 11 12:36:41 PDT 2002
//   I changed the code so it works on Windows.
//
//   Brad Whitlock, Mon Feb 10 11:32:50 PDT 2003
//   Made it work on Windows.
//
//   Brad Whitlock, Thu Oct 30 14:21:57 PST 2003
//   I made uchar print as ints.
//
//   Mark C. Miller, Tue Jan 18 12:44:34 PST 2005
//   Added checks for null pointers before inserting (<<)
//
//   Brad Whitlock, Thu Feb 24 16:05:29 PST 2005
//   Fixed for win32.
//
// ****************************************************************************

ostream &
operator << (ostream& os, const AttributeGroup& atts)
{
    static const char* indentSpace[] = {"",
                                        "",
                                        "   ",
                                        "      ",
                                        "         ",
                                        "            ",
                                        "               ",
                                        "                  ",
                                        "                     ",
                                        "                        "};
    bool isRecursive = false;
    int i,k;

    indentLevel++;

    // handle indentation for recursion
    os << indentSpace[indentLevel];

    // output a header
    os << atts.TypeName().c_str() << " has " << atts.NumAttributesSelected() <<
       " of " << atts.NumAttributes() << " fields selected" << endl;

    // output a line for each field. We use low-level typeInfoVector
    // stuff because we need to get at actual values too. But, we need
    // high-level GetFieldType|Name API, too. So, we're using both here
    AttributeGroup::typeInfoVector::const_iterator pos;
    k = 0;
    for(pos = atts.typeMap.begin(); pos != atts.typeMap.end(); ++pos)
    {

        // output beginning of line for this field
        os << indentSpace[indentLevel];

        // output selection status too
        if (pos->selected && !isRecursive)
            os << "    +" << '(' << k << ')';
        else
            os << "     " << '(' << k << ')';

        os << atts.GetFieldTypeName(k).c_str() << " "
           << atts.GetFieldName(k).c_str() << " = ";

        // output the "value" for this field
        switch(pos->typeCode)
        {

        // primitive types
        case msgTypeChar:
            {   char *cptr = (char *)(pos->address);
                if (cptr)
                    os << *cptr;
            }
            break;
        case msgTypeUnsignedChar:
            {   unsigned char *ucptr = (unsigned char *)(pos->address);
                if (ucptr)
                    os << *ucptr;
            }
            break;
        case msgTypeInt:
            {   int *iptr = (int *)(pos->address);
                if (iptr)
                    os << *iptr;
            }
            break;
        case msgTypeLong:
            {   long *lptr = (long *)(pos->address);
                if (lptr)
                    os << *lptr;
            }
            break;
        case msgTypeFloat:
            {   float *fptr = (float *)(pos->address);
                if (fptr)
                    os << *fptr;
            }
            break;
        case msgTypeDouble:
            {   double *dptr = (double *)(pos->address);
                if (dptr)
                    os << *dptr;
            }
            break;
        case msgTypeString:
            {   vtkstd::string *sptr = (vtkstd::string *)(pos->address);
                if (sptr)
                    os << sptr->c_str();
            }
            break;
        case msgTypeAttributeGroup:
            // Note: recursive call to << operator
            isRecursive = true;
            if(pos->selected)
                os << " <Attr> is selected" << endl;
            else
                os << " <Attr> is NOT selected" << endl;
            os << *((AttributeGroup *)(pos->address));
            break;
        case msgTypeBool:
            os << (*((bool *)(pos->address)) ? "true" : "false");
            break;

        // lists of primitive types
        case msgTypeListChar:
            {   char *cptr = (char *) (pos->address);
                for(i = 0; i < pos->length; ++i)
                    os << ", '" << cptr[i] << "'";
            }
            break;
        case msgTypeListUnsignedChar:
            {   unsigned char *ucptr = (unsigned char *) (pos->address);
                for(i = 0; i < pos->length; ++i)
                    os << ", " << int(ucptr[i]);
            }
            break;
        case msgTypeListInt:
            {   int *iptr = (int *) (pos->address);
                for(i = 0; i < pos->length; ++i)
                    os << ", " << iptr[i];
            }
            break;
        case msgTypeListLong:
            {   long *lptr = (long *) (pos->address);
                for(i = 0; i < pos->length; ++i)
                    os << ", " << lptr[i];
            }
            break;
        case msgTypeListFloat:
            {   float *fptr = (float *) (pos->address);
                for(i = 0; i < pos->length; ++i)
                    os << ", " << fptr[i];
            }
            break;
        case msgTypeListDouble:
            {   double *dptr = (double *) (pos->address);
                for(i = 0; i < pos->length; ++i)
                    os << ", " << dptr[i];
            }
            break;
        case msgTypeListString:
            {   vtkstd::string *sptr = (vtkstd::string *)(pos->address);
                for(i = 0; i < pos->length; ++i)
                    os << ", " << sptr[i].c_str();
            }
            break;
        case msgTypeListAttributeGroup:
            // Note: recursive call to << operator
            isRecursive = true;
            if(pos->selected)
                os << " <Attr> is selected" << endl;
            else
                os << " <Attr> is NOT selected" << endl;
            {   AttributeGroup **aptr = (AttributeGroup **)(pos->address);
                for(i = 0; i < pos->length; ++i)
                    os << *(aptr[i]);
            }
            break;
        case msgTypeListBool:
            {   bool *bptr = (bool *)(pos->address);
                for(i = 0; i < pos->length; ++i)
                    os << ", " << bptr[i];
            }
            break;

        // vectors of primitive types
        case msgTypeVectorBool:
            {   boolVector *vb = (boolVector *)(pos->address);
                boolVector::iterator bpos;
                for(bpos = vb->begin(); bpos != vb->end(); ++bpos)
                    os << ", '" << (*bpos==1?"true":"false") << "'";
            }
            break;
        case msgTypeVectorChar:
            {   charVector *vc = (charVector *)(pos->address);
                charVector::iterator cpos;
                for(cpos = vc->begin(); cpos != vc->end(); ++cpos)
                    os << ", '" << *cpos << "'";
            }
            break;
        case msgTypeVectorUnsignedChar:
            {   unsignedCharVector *vc = (unsignedCharVector *)(pos->address);
                unsignedCharVector::iterator cpos;
                for(cpos = vc->begin(); cpos != vc->end(); ++cpos)
                    os << ", " << int(*cpos);
            }
            break;
        case msgTypeVectorInt:
            {   intVector *vi = (intVector *)(pos->address);
                intVector::iterator ipos;
                for(ipos = vi->begin(); ipos != vi->end(); ++ipos)
                    os << ", " << *ipos;
            }
            break;
        case msgTypeVectorLong:
            {   longVector *vl = (longVector *)(pos->address);
                longVector::iterator lpos;
                for(lpos = vl->begin(); lpos != vl->end(); ++lpos)
                    os << ", " << *lpos;
            }
            break;
        case msgTypeVectorFloat:
            {   floatVector *vf = (floatVector *)(pos->address);
                floatVector::iterator fpos;
                for(fpos = vf->begin(); fpos != vf->end(); ++fpos)
                    os << ", " << *fpos;
            }
            break;
        case msgTypeVectorDouble:
            {   doubleVector *vd = (doubleVector *)(pos->address);
                doubleVector::iterator dpos;
                for(dpos = vd->begin(); dpos != vd->end(); ++dpos)
                    os << ", " << *dpos;
            }
            break;
        case msgTypeVectorString:
            {   stringVector *vs = (stringVector *)(pos->address);
                stringVector::iterator spos;
                for(spos = vs->begin(); spos != vs->end(); ++spos)
                    os << ", " << spos->c_str();
            }
            break;
        case msgTypeVectorAttributeGroup:
            // Note: recursive call to << operator
            isRecursive = true;
            if(pos->selected)
                os << " <Attr> is selected" << endl;
            else
                os << " <Attr> is NOT selected" << endl;
            {   AttributeGroupVector *va = (AttributeGroupVector *)(pos->address);
                AttributeGroupVector::iterator apos;
                for(apos = va->begin(); apos != va->end(); ++apos)
                    os << *apos;
            }
            break;
        case msgTypeNone:
        default:
            ; // nothing.
        }

        os << endl;

        k++;
    }

    indentLevel--;

    return os;
}
