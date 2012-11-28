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

#include <string.h>
#include <DataNode.h>

// Static members. These are returned from the As functions when there
// is no data. It ensures that the references that are returned are safe.
std::string         DataNode::bogusString;
charVector          DataNode::bogusCharVector;
unsignedCharVector  DataNode::bogusUnsignedCharVector;
intVector           DataNode::bogusIntVector;
longVector          DataNode::bogusLongVector;
floatVector         DataNode::bogusFloatVector;
doubleVector        DataNode::bogusDoubleVector;
stringVector        DataNode::bogusStringVector;

// ****************************************************************************
// Method: DataNode::DataNode
//
// Purpose: 
//   These are the constructors for the DataNode class. There is one
//   constructor for each type of node.
//
// Programmer: Brad Whitlock
// Creation:   Thu Sep 28 12:22:58 PDT 2000
//
// Modifications:
//    Jeremy Meredith, Mon Feb 26 16:01:24 PST 2001
//    Added unsigned chars.
//   
// ****************************************************************************

DataNode::DataNode(const std::string &name) : Key(name)
{
    NodeType = INTERNAL_NODE;
    Length = 0;
    Data = 0;
}

DataNode::DataNode(const std::string &name, char val) : Key(name)
{
    NodeType = CHAR_NODE;
    Length = 0;
    Data = (void *)(new char(val));
}

DataNode::DataNode(const std::string &name, unsigned char val) : Key(name)
{
    NodeType = UNSIGNED_CHAR_NODE;
    Length = 0;
    Data = (void *)(new unsigned char(val));
}

DataNode::DataNode(const std::string &name, int val) : Key(name)
{
    NodeType = INT_NODE;
    Length = 0;
    Data = (void *)(new int(val));
}

DataNode::DataNode(const std::string &name, long val) : Key(name)
{
    NodeType = LONG_NODE;
    Length = 0;
    Data = (void *)(new long(val));
}

DataNode::DataNode(const std::string &name, float val) : Key(name)
{
    NodeType = FLOAT_NODE;
    Length = 0;
    Data = (void *)(new float(val));
}

DataNode::DataNode(const std::string &name, double val) : Key(name)
{
    NodeType = DOUBLE_NODE;
    Length = 0;
    Data = (void *)(new double(val));
}

DataNode::DataNode(const std::string &name, const std::string &val) : Key(name)
{
    NodeType = STRING_NODE;
    Length = 0;
    Data = (void *)(new std::string(val));
}

DataNode::DataNode(const std::string &name, bool val) : Key(name)
{
    NodeType = BOOL_NODE;
    Length = 0;
    Data = (void *)(new bool(val));
}

DataNode::DataNode(const std::string &name, const char *vals, int len) : Key(name)
{
    NodeType = CHAR_ARRAY_NODE;
    Length = len;
    if(len > 0)
    {
        Data = (void *)(new char[len]);
        memcpy(Data, (void *)vals, len * sizeof(char));
    }
    else
        Data = 0;
}

DataNode::DataNode(const std::string &name, const unsigned char *vals, int len) : Key(name)
{
    NodeType = UNSIGNED_CHAR_ARRAY_NODE;
    Length = len;
    if(len > 0)
    {
        Data = (void *)(new unsigned char[len]);
        memcpy(Data, (void *)vals, len * sizeof(unsigned char));
    }
    else
        Data = 0;
}

DataNode::DataNode(const std::string &name, const int *vals, int len) : Key(name)
{
    NodeType = INT_ARRAY_NODE;
    Length = len;
    if(len > 0)
    {
        Data = (void *)(new int[len]);
        memcpy(Data, (void *)vals, len * sizeof(int));
    }
    else
        Data = 0;
}

DataNode::DataNode(const std::string &name, const long *vals, int len) : Key(name)
{
    NodeType = LONG_ARRAY_NODE;
    Length = len;
    if(len > 0)
    {
        Data = (void *)(new long[len]);
        memcpy(Data, (void *)vals, len * sizeof(long));
    }
    else
        Data = 0;
}

DataNode::DataNode(const std::string &name, const float *vals, int len) : Key(name)
{
    NodeType = FLOAT_ARRAY_NODE;
    Length = len;
    if(len > 0)
    {
        Data = (void *)(new float[len]);
        memcpy(Data, (void *)vals, len * sizeof(float));
    }
    else
        Data = 0;
}

DataNode::DataNode(const std::string &name, const double *vals, int len) : Key(name)
{
    NodeType = DOUBLE_ARRAY_NODE;
    Length = len;
    if(len > 0)
    {
        Data = (void *)(new double[len]);
        memcpy(Data, (void *)vals, len * sizeof(double));
    }
    else
        Data = 0;
}

DataNode::DataNode(const std::string &name, const std::string *vals, int len) : Key(name)
{
    NodeType = STRING_ARRAY_NODE;
    Length = len;
    if(len > 0)
    {
        std::string *sptr = new std::string[len];
        Data = (void *)sptr;   
        for(int i = 0; i < len; ++i)
            sptr[i] = vals[i];
    }
    else
        Data = 0;
}

DataNode::DataNode(const std::string &name, const bool *vals, int len) : Key(name)
{
    NodeType = BOOL_ARRAY_NODE;
    Length = len;
    if(len > 0)
    {
        Data = (void *)(new bool[len]);
        memcpy(Data, (void *)vals, len * sizeof(bool));
    }
    else
        Data = 0;
}

DataNode::DataNode(const std::string &name, const charVector &vec) : Key(name)
{
    NodeType = CHAR_VECTOR_NODE;
    Length = 0;
    Data = (void *)(new charVector(vec));
}

DataNode::DataNode(const std::string &name, const unsignedCharVector &vec) : Key(name)
{
    NodeType = UNSIGNED_CHAR_VECTOR_NODE;
    Length = 0;
    Data = (void *)(new unsignedCharVector(vec));
}

DataNode::DataNode(const std::string &name, const intVector &vec) : Key(name)
{
    NodeType = INT_VECTOR_NODE;
    Length = 0;
    Data = (void *)(new intVector(vec));
}

DataNode::DataNode(const std::string &name, const longVector &vec) : Key(name)
{
    NodeType = LONG_VECTOR_NODE;
    Length = 0;
    Data = (void *)(new longVector(vec));
}

DataNode::DataNode(const std::string &name, const floatVector &vec) : Key(name)
{
    NodeType = FLOAT_VECTOR_NODE;
    Length = 0;
    Data = (void *)(new floatVector(vec));
}

DataNode::DataNode(const std::string &name, const doubleVector &vec) : Key(name)
{
    NodeType = DOUBLE_VECTOR_NODE;
    Length = 0;
    Data = (void *)(new doubleVector(vec));
}

DataNode::DataNode(const std::string &name, const stringVector &vec) : Key(name)
{
    NodeType = STRING_VECTOR_NODE;
    Length = 0;
    Data = (void *)(new stringVector(vec));
}

// ****************************************************************************
// Method: DataNode::~DataNode
//
// Purpose: 
//   Destructor for the DataNode class. It frees any data that the node
//   has based on the type of the node.
//
// Programmer: Brad Whitlock
// Creation:   Thu Sep 28 12:22:11 PDT 2000
//
// Modifications:
//   Jeremy Meredith, Mon Feb 26 16:01:37 PST 2001
//   Added unsigned chars.
//
//   Brad Whitlock, Fri Mar 21 10:48:13 PDT 2003
//   I moved the implementation into FreeData.
//
// ****************************************************************************

DataNode::~DataNode()
{
    FreeData();
}

// ****************************************************************************
// Method: DataNode::FreeData
//
// Purpose: 
//   Frees the data in the DataNode and sets the Data pointer to 0.
//
// Programmer: Brad Whitlock
// Creation:   Fri Mar 21 10:47:29 PDT 2003
//
// Modifications:
//   
// ****************************************************************************

void
DataNode::FreeData()
{
    if(Data == 0)
        return;

    // Any node types that have dynamically allocated storage must
    // be freed specially.
    switch(NodeType)
    {
    case INTERNAL_NODE:
        { // new scope
            if(Length == 1)
            {
                // Delete the child.
                DataNode *node = (DataNode *)Data;
                delete node;
            }
            else if(Length > 1)
            {
                // Delete all the children
                DataNode **nodeArray = (DataNode **)Data;
                for(int i = 0; i < Length; ++i)
                    delete nodeArray[i];

                // Delete the children's pointer array
                if(Length > 1)
                    delete [] nodeArray;
            }
        }
        break;
    case CHAR_NODE:
        { // new scope
            char *cptr = (char *)Data;
            delete cptr;
        }
        break;        
    case UNSIGNED_CHAR_NODE:
        { // new scope
            unsigned char *uptr = (unsigned char *)Data;
            delete uptr;
        }
        break;        
    case INT_NODE:
        { // new scope
            int *iptr = (int *)Data;
            delete iptr;
        }
        break;        
    case LONG_NODE:
        { // new scope
            long *lptr = (long *)Data;
            delete lptr;
        }
        break;        
    case FLOAT_NODE:
        { // new scope
            float *fptr = (float *)Data;
            delete fptr;
        }
        break;
    case DOUBLE_NODE:
        { // new scope
            double *dptr = (double *)Data;
            delete dptr;
        }
        break;
    case STRING_NODE:
        { // new scope
            std::string *sptr = (std::string *)Data;
            delete sptr;
        }
        break;
    case BOOL_NODE:
        { // new scope
            bool *bptr = (bool *)Data;
            delete bptr;
        }
        break;        
    case CHAR_ARRAY_NODE:
        { // new scope
            char *cptr = (char *)Data;
            delete [] cptr;
        }
        break;        
    case UNSIGNED_CHAR_ARRAY_NODE:
        { // new scope
            unsigned char *uptr = (unsigned char *)Data;
            delete [] uptr;
        }
        break;        
    case INT_ARRAY_NODE:
        { // new scope
            int *iptr = (int *)Data;
            delete [] iptr;
        }
        break;        
    case LONG_ARRAY_NODE:
        { // new scope
            long *lptr = (long *)Data;
            delete [] lptr;
        }
        break;        
    case FLOAT_ARRAY_NODE:
        { // new scope
            float *fptr = (float *)Data;
            delete [] fptr;
        }
        break;
    case DOUBLE_ARRAY_NODE:
        { // new scope
            double *dptr = (double *)Data;
            delete [] dptr;
        }
        break;
    case STRING_ARRAY_NODE:
        { // new scope
            std::string *sptr = (std::string *)Data;
            delete [] sptr;
        }
        break;
    case BOOL_ARRAY_NODE:
        { // new scope
            bool *bptr = (bool *)Data;
            delete [] bptr;
        }
        break;
    case CHAR_VECTOR_NODE:
        { // new scope
            charVector *cptr = (charVector *)Data;
            delete cptr;
        }
        break;        
    case UNSIGNED_CHAR_VECTOR_NODE:
        { // new scope
            unsignedCharVector *uptr = (unsignedCharVector *)Data;
            delete uptr;
        }
        break;        
    case INT_VECTOR_NODE:
        { // new scope
            intVector *iptr = (intVector *)Data;
            delete iptr;
        }
        break;        
    case LONG_VECTOR_NODE:
        { // new scope
            longVector *lptr = (longVector *)Data;
            delete lptr;
        }
        break;        
    case FLOAT_VECTOR_NODE:
        { // new scope
            floatVector *fptr = (floatVector *)Data;
            delete fptr;
        }
        break;
    case DOUBLE_VECTOR_NODE:
        { // new scope
            doubleVector *dptr = (doubleVector *)Data;
            delete dptr;
        }
        break;
    case STRING_VECTOR_NODE:
        { // new scope
            stringVector *sptr = (stringVector *)Data;
            delete sptr;
        }
        break;
    case BOOL_VECTOR_NODE:
        // Do nothing since it can't be instantiated.
        break;
    }

    Data = 0;
    Length = 0;
}

//
// Methods to return the data as a value of a certain type.
//

char
DataNode::AsChar() const
{
    return *((char *)Data);
}

unsigned char
DataNode::AsUnsignedChar() const
{
    return *((unsigned char *)Data);
}

int
DataNode::AsInt() const
{
    return *((int *)Data);
}

long
DataNode::AsLong() const
{
    return *((long *)Data);
}

float
DataNode::AsFloat() const
{
    float rv = 0.f;
    if (NodeType == FLOAT_NODE)
        rv = *((float *)Data);
    else if (NodeType == DOUBLE_NODE)
        rv = float(*((double *)Data));
    return rv;
}

double
DataNode::AsDouble() const
{
    double rv = 0.;
    if (NodeType == DOUBLE_NODE)
        rv =  *((double *)Data);
    else if (NodeType == FLOAT_NODE)
        rv = double(*((float*)Data));
    return rv;
}

const std::string &
DataNode::AsString() const
{
    if(NodeType == STRING_NODE && Data != 0)
        return *((std::string *)Data);
    else
        return bogusString;
}

bool
DataNode::AsBool() const
{
    return *((bool *)Data);
}

const char *
DataNode::AsCharArray() const
{
    return (const char *)Data;
}

const unsigned char *
DataNode::AsUnsignedCharArray() const
{
    return (const unsigned char *)Data;
}

const int *
DataNode::AsIntArray() const
{
    return (const int *)Data;
}

const long *
DataNode::AsLongArray() const
{
    return (const long *)Data;
}

const float *
DataNode::AsFloatArray() const
{
    return (const float *)Data;
}

const double *
DataNode::AsDoubleArray() const
{
    return (const double *)Data;
}

const std::string *
DataNode::AsStringArray() const
{
    return (const std::string *)Data;
}

const bool *
DataNode::AsBoolArray() const
{
    return (const bool *)Data;
}

const charVector &
DataNode::AsCharVector() const
{
    if(NodeType == CHAR_VECTOR_NODE && Data != 0)
        return *((charVector *)Data);
    else
        return bogusCharVector;
}

const unsignedCharVector &
DataNode::AsUnsignedCharVector() const
{
    if(NodeType == UNSIGNED_CHAR_VECTOR_NODE && Data != 0)
        return *((unsignedCharVector *)Data);
    else
        return bogusUnsignedCharVector;
}

const intVector &
DataNode::AsIntVector() const
{
    if(NodeType == INT_VECTOR_NODE && Data != 0)
        return *((intVector *)Data);
    else
        return bogusIntVector;
}

const longVector &
DataNode::AsLongVector() const
{
    if(NodeType == LONG_VECTOR_NODE && Data != 0)
        return *((longVector *)Data);
    else
        return bogusLongVector;
}

const floatVector &
DataNode::AsFloatVector() const
{
    if(NodeType == FLOAT_VECTOR_NODE && Data != 0)
        return *((floatVector *)Data);
    else
        return bogusFloatVector;
}

const doubleVector &
DataNode::AsDoubleVector() const
{
    if(NodeType == DOUBLE_VECTOR_NODE && Data != 0)
        return *((doubleVector *)Data);
    else
        return bogusDoubleVector;
}

const stringVector &
DataNode::AsStringVector() const
{
    if(NodeType == STRING_VECTOR_NODE && Data != 0)
        return *((stringVector *)Data);
    else
        return bogusStringVector;
}

//
// Methods to convert the DataNode to a new type with new data.
//

void
DataNode::SetChar(char val)
{
    FreeData();
    NodeType = CHAR_NODE;
    Data = (void *)(new char(val));
}

void
DataNode::SetUnsignedChar(unsigned char val)
{
    FreeData();
    NodeType = UNSIGNED_CHAR_NODE;
    Data = (void *)(new unsigned char(val));
}

void
DataNode::SetInt(int val)
{
    FreeData();
    NodeType = INT_NODE;
    Data = (void *)(new int(val));
}

void
DataNode::SetLong(long val)
{
    FreeData();
    NodeType = LONG_NODE;
    Data = (void *)(new long(val));
}

void
DataNode::SetFloat(float val)
{
    FreeData();
    NodeType = FLOAT_NODE;
    Data = (void *)(new float(val));
}

void
DataNode::SetDouble(double val)
{
    FreeData();
    NodeType = DOUBLE_NODE;
    Data = (void *)(new double(val));
}

void
DataNode::SetString(const std::string &val)
{
    FreeData();
    NodeType = STRING_NODE;
    Data = (void *)(new std::string(val));
}

void
DataNode::SetBool(bool val)
{
    FreeData();
    NodeType = BOOL_NODE;
    Data = (void *)(new bool(val));
}

void
DataNode::SetCharArray(const char *vals, int len)
{
    FreeData();
    NodeType = CHAR_ARRAY_NODE;
    Length = len;
    if(len > 0)
    {
        Data = (void *)(new char[len]);
        memcpy(Data, (void *)vals, len * sizeof(char));
    }
    else
        Data = 0;
}

void
DataNode::SetUnsignedCharArray(const unsigned char *vals, int len)
{
    FreeData();
    NodeType = UNSIGNED_CHAR_ARRAY_NODE;
    Length = len;
    if(len > 0)
    {
        Data = (void *)(new unsigned char[len]);
        memcpy(Data, (void *)vals, len * sizeof(unsigned char));
    }
    else
        Data = 0;
}

void
DataNode::SetIntArray(const int *vals, int len)
{
    FreeData();
    NodeType = INT_ARRAY_NODE;
    Length = len;
    if(len > 0)
    {
        Data = (void *)(new int[len]);
        memcpy(Data, (void *)vals, len * sizeof(int));
    }
    else
        Data = 0;
}

void
DataNode::SetLongArray(const long *vals, int len)
{
    FreeData();
    NodeType = LONG_ARRAY_NODE;
    Length = len;
    if(len > 0)
    {
        Data = (void *)(new long[len]);
        memcpy(Data, (void *)vals, len * sizeof(long));
    }
    else
        Data = 0;
}

void
DataNode::SetFloatArray(const float *vals, int len)
{
    FreeData();
    NodeType = FLOAT_ARRAY_NODE;
    Length = len;
    if(len > 0)
    {
        Data = (void *)(new float[len]);
        memcpy(Data, (void *)vals, len * sizeof(float));
    }
    else
        Data = 0;
}

void
DataNode::SetDoubleArray(const double *vals, int len)
{
    FreeData();
    NodeType = DOUBLE_ARRAY_NODE;
    Length = len;
    if(len > 0)
    {
        Data = (void *)(new double[len]);
        memcpy(Data, (void *)vals, len * sizeof(double));
    }
    else
        Data = 0;
}

void
DataNode::SetStringArray(const std::string *vals, int len)
{
    FreeData();
    NodeType = STRING_ARRAY_NODE;
    Length = len;
    if(len > 0)
    {
        std::string *sptr = new std::string[len];
        Data = (void *)sptr;   
        for(int i = 0; i < len; ++i)
            sptr[i] = vals[i];
    }
    else
        Data = 0;
}

void
DataNode::SetBoolArray(const bool *vals, int len)
{
    FreeData();
    NodeType = BOOL_ARRAY_NODE;
    Length = len;
    if(len > 0)
    {
        Data = (void *)(new bool[len]);
        memcpy(Data, (void *)vals, len * sizeof(bool));
    }
    else
        Data = 0;
}

void
DataNode::SetCharVector(const charVector &vec)
{
    FreeData();
    NodeType = CHAR_VECTOR_NODE;
    Data = (void *)(new charVector(vec));
}

void
DataNode::SetUnsignedCharVector(const unsignedCharVector &vec)
{
    FreeData();
    NodeType = UNSIGNED_CHAR_VECTOR_NODE;
    Data = (void *)(new unsignedCharVector(vec));
}

void
DataNode::SetIntVector(const intVector &vec)
{
    FreeData();
    NodeType = INT_VECTOR_NODE;
    Data = (void *)(new intVector(vec));
}

void
DataNode::SetLongVector(const longVector &vec)
{
    FreeData();
    NodeType = LONG_VECTOR_NODE;
    Data = (void *)(new longVector(vec));
}

void
DataNode::SetFloatVector(const floatVector &vec)
{
    FreeData();
    NodeType = FLOAT_VECTOR_NODE;
    Data = (void *)(new floatVector(vec));
}

void
DataNode::SetDoubleVector(const doubleVector &vec)
{
    FreeData();
    NodeType = DOUBLE_VECTOR_NODE;
    Data = (void *)(new doubleVector(vec));
}

void
DataNode::SetStringVector(const stringVector &vec)
{
    FreeData();
    NodeType = STRING_VECTOR_NODE;
    Data = (void *)(new stringVector(vec));
}

// ****************************************************************************
// Method: DataNode::GetNode
//
// Purpose: 
//   Returns a pointer to the node having the specified key. If a
//   parentNode is supplied, then only the children of that node are
//   searched.
//
// Arguments:
//   key : The name of the node to look for.
//   parentNode : The root of the tree to search.
//
// Returns:    
//   A pointer to the node having the specified key, or 0 if the node
//   is not found.
//
// Programmer: Brad Whitlock
// Creation:   Thu Sep 28 12:07:56 PDT 2000
//
// Modifications:
//   
// ****************************************************************************

DataNode *
DataNode::GetNode(const std::string &key, DataNode *parentNode)
{
    DataNode *searchNode, *intermediate, *retval = 0;

    // Determine which node's children to search.        
    if(parentNode == 0)
        searchNode = this;
    else
        searchNode = parentNode;

    if(key == searchNode->Key)
        retval = searchNode;
    else if(searchNode->NodeType == INTERNAL_NODE)
    {
        if(searchNode->Length == 1)
        {
            DataNode *nodeArray = (DataNode *)(searchNode->Data);
            intermediate = GetNode(key, nodeArray);
            if(intermediate != 0)
            {
                retval = intermediate;
            }
        }
        else if(searchNode->Length > 1)
        {
            DataNode **nodeArray = (DataNode **)(searchNode->Data);

            for(int i = 0; i < searchNode->Length; ++i)
            {
                intermediate = GetNode(key, nodeArray[i]);
                if(intermediate != 0)
                {
                    retval = intermediate;
                       break;
                }
            }
        }
    }

    return retval;
}

// ****************************************************************************
// Method: DataNode::AddNode
//
// Purpose: 
//   Adds a child node to the current node if the current node is of
//   type INTERNAL_NODE.
//
// Arguments:
//   node : A pointer to the node that will be added.
//
// Programmer: Brad Whitlock
// Creation:   Thu Sep 28 12:12:38 PDT 2000
//
// Modifications:
//   Brad Whitlock, Fri Feb 1 13:27:33 PST 2002
//   Fixed a memory leak.
//
// ****************************************************************************

void
DataNode::AddNode(DataNode *node)
{
    if(NodeType != INTERNAL_NODE || node == 0)
        return;

    int i;
    if(Length == 0)
    {
        Length = 1;
        Data = (void *)node;
    }
    else if(Length == 1)
    {
        DataNode **nodeArray = new DataNode*[2];
        nodeArray[0] = (DataNode *)Data;
        nodeArray[1] = node;
        Data = (void *)nodeArray;
        Length = 2;
    }
    else
    {
        DataNode **nodeArray = new DataNode*[Length + 1];
        DataNode **dNptr = (DataNode **)Data;
        for(i = 0; i < Length; ++i)
            nodeArray[i] = dNptr[i];
        nodeArray[i] = node;
        delete [] dNptr;
        Data = (void *)nodeArray;
        ++Length;
    }
}

// ****************************************************************************
// Method: DataNode::RemoveNode
//
// Purpose: 
//   Removes the specified node if it exists under the current node.
//
// Arguments:
//   node : The node to remove.
//
// Programmer: Eric Brugger
// Creation:   Tue Mar 27 15:57:42 PDT 2007
//
// Modifications:
//
// ****************************************************************************

void
DataNode::RemoveNode(DataNode *node, bool deleteNode)
{
    if(NodeType != INTERNAL_NODE)
        return;
    if(Length < 1)
        return;

    if(Length == 1)
    {
        if((DataNode *)Data == node)
        {
            if(deleteNode)
                delete node;
            Data = 0;
            Length = 0;
        }
    }
    else
    {
        DataNode **nodeArray = (DataNode **)Data;
        bool start = false;

        for(int i = 0; i < Length; ++i)
        {
            if(!start && nodeArray[i] == node)
            {
                if(deleteNode)
                    delete nodeArray[i];
                start = true;
            }

            if(start && (i < (Length - 1)))
                nodeArray[i] = nodeArray[i + 1];
        }
        if(start)
        {
            --Length;

            // If we're down to 1, convert to a single pointer.
            if(Length == 1)
            {
                DataNode *temp = nodeArray[0];
                delete [] nodeArray;
                Data = (void *)temp;
            } 
        }
    }
}

// ****************************************************************************
// Method: DataNode::RemoveNode
//
// Purpose: 
//   Removes the node with the specified key if it exists under the 
//   current node.
//
// Arguments:
//   key : The key of the node to remove.
//
// Programmer: Brad Whitlock
// Creation:   Thu Sep 28 12:14:24 PDT 2000
//
// Modifications:
//   Brad Whitlock, Mon Feb 2 15:38:05 PST 2004
//   I added an optional argument to delete the node.
//
// ****************************************************************************

void
DataNode::RemoveNode(const std::string &key, bool deleteNode)
{
    if(NodeType != INTERNAL_NODE)
        return;
    if(Length < 1)
        return;

    if(Length == 1)
    {
        DataNode *node = (DataNode *)Data;
        if(node->Key == key)
        {
            if(deleteNode)
                delete node;
            Data = 0;
            Length = 0;
        }
    }
    else
    {
        DataNode **nodeArray = (DataNode **)Data;
        bool start = false;

        for(int i = 0; i < Length; ++i)
        {
            if(!start && nodeArray[i]->Key == key)
            {
                if(deleteNode)
                    delete nodeArray[i];
                start = true;
            }

            if(start && (i < (Length - 1)))
                nodeArray[i] = nodeArray[i + 1];
        }
        if(start)
        {
            --Length;

            // If we're down to 1, convert to a single pointer.
            if(Length == 1)
            {
                DataNode *temp = nodeArray[0];
                delete [] nodeArray;
                Data = (void *)temp;
            } 
        }
    }
}

//
// Methods to get/set some of the private fields.
//

const std::string &
DataNode::GetKey() const
{
    return Key;
}

void
DataNode::SetKey(const std::string &k)
{
    Key = k;
}

NodeTypeEnum
DataNode::GetNodeType() const
{
    return NodeType;
}

int
DataNode::GetLength() const
{
    return Length;
}

int
DataNode::GetNumChildren() const
{
    return Length;
}

// ****************************************************************************
// Method: DataNode::GetNumChildObjects
//
// Purpose: 
//   Return the number of children that are of type INTERNAL_NODE
//
// Programmer: Brad Whitlock
// Creation:   Fri Sep 29 17:54:21 PST 2000
//
// Modifications:
//   
// ****************************************************************************

int
DataNode::GetNumChildObjects() const
{
    int retval = 0;

    if(Length == 1)
    {
        DataNode *child = (DataNode *)Data;
        if(child->NodeType == INTERNAL_NODE)
            retval = 1;
    }
    else if(Length > 0)
    {
        DataNode **children = (DataNode **)Data;
        for(int i = 0; i < Length; ++i)
        {
            if(children[i]->NodeType == INTERNAL_NODE)
                ++retval;
        }
    }

    return retval;
}

// ****************************************************************************
// Method: DataNode::GetChildren
//
// Purpose: 
//   Returns an array of DataNode pointers that point to the node's
//   children. If there are no children, 0 is returned.
//
// Programmer: Brad Whitlock
// Creation:   Thu Sep 28 15:11:24 PST 2000
//
// Modifications:
//   
// ****************************************************************************

DataNode **
DataNode::GetChildren()
{
    if(NodeType == INTERNAL_NODE)
    {
        if(Length == 0)
            return 0;
        else if(Length == 1)
            return ((DataNode **)&Data);
        else
            return ((DataNode **)Data);
    }
    else
        return 0;
}

//
// Enum name lookup stuff.
//

static const char *NodeTypeNameLookup[] = {
"",
"char", "unsigned char", "int", "long", "float",
"double", "string", "bool",
"charArray", "unsignedCharArray", "intArray", "longArray", "floatArray",
"doubleArray", "stringArray", "boolArray",
"charVector", "unsignedCharVector", "intVector", "longVector", "floatVector",
"doubleVector", "stringVector"
};

const char *
NodeTypeName(NodeTypeEnum e)
{
    return NodeTypeNameLookup[(int)e];
}

// ****************************************************************************
// Function: GetNodeType
//
// Purpose: 
//   Converts a named type to a NodeTypeEnum.
//
// Returns:    The NodeTypeEnum corresponding to the named type.
//
// Programmer: Brad Whitlock
// Creation:   Mon Jun 18 23:30:52 PST 2001
//
// Modifications:
//   
// ****************************************************************************

NodeTypeEnum
GetNodeType(const char *str)
{
    bool found = false;
    int retval = 0;

    for(int i = 1; i < 24 && !found; ++i)
    {
        found = (strcmp(str, NodeTypeNameLookup[i]) == 0);
        if(found)
            retval = i;
    }

    return (NodeTypeEnum)retval;
}
