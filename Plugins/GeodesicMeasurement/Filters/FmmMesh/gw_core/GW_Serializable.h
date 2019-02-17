/*------------------------------------------------------------------------------*/
/**
 *  \file   GW_Serializable.h
 *  \brief  Definition of class \c GW_Serializable
 *  \author Gabriel Peyré
 *  \date   4-2-2003
 */
/*------------------------------------------------------------------------------*/

#ifndef _GW_SERIALIZABLE_H_
#define _GW_SERIALIZABLE_H_

#include "GW_Config.h"

namespace GW {

class FMMMESH_EXPORT GW_Ofstream: public std::ofstream
{

public:

    #ifdef std::_Ios_Openmode
        #define my_openmode std::_Ios_Openmode
    #else
        #define my_openmode GW_I32
    #endif

    /** constructor */
    GW_Ofstream(const char* name, my_openmode mode = std::ios_base::binary | std::ios_base::trunc)
    :std::ofstream(name, mode){}

    /** destructor */
    virtual ~GW_Ofstream(){}

    /** operators */
    #define DEFINE_OPERATOR(type)                \
    GW_Ofstream& operator << (type& v)            \
    { this->write( (char*) &v, (int) sizeof(type) );    \
    return *this; }

    DEFINE_OPERATOR(GW_I8);
    DEFINE_OPERATOR(GW_U8);
    DEFINE_OPERATOR(GW_I16);
    DEFINE_OPERATOR(GW_U16);
    DEFINE_OPERATOR(GW_I32);
    DEFINE_OPERATOR(GW_U32);
    //DEFINE_OPERATOR(GW_I64);
    //DEFINE_OPERATOR(GW_U64);
    DEFINE_OPERATOR(GW_Bool);
    DEFINE_OPERATOR(GW_Float);

    #undef DEFINE_OPERATOR

    /** special treatment for strings */
    GW_Ofstream& operator << (string& v)
    {
        GW_U32 nSize = (GW_U32) v.size();
        (*this) << nSize;
        this->write( (char*) v.c_str(), (int) v.size() );
        return *this;
    }
};

class GW_Ifstream: public std::ifstream
{

public:

    GW_Ifstream(const char* name, my_openmode mode = std::ios_base::binary)
    :std::ifstream(name, mode){}
    virtual ~GW_Ifstream(){}

    #define DEFINE_OPERATOR(type)                    \
    GW_Ifstream& operator >> (type& v)            \
    { this->read( (char*) &v, (int) sizeof(type) );    \
    return *this; }

    DEFINE_OPERATOR(GW_I8);
    DEFINE_OPERATOR(GW_U8);
    DEFINE_OPERATOR(GW_I16);
    DEFINE_OPERATOR(GW_U16);
    DEFINE_OPERATOR(GW_I32);
    DEFINE_OPERATOR(GW_U32);
//    DEFINE_OPERATOR(GW_I64);
//    DEFINE_OPERATOR(GW_U64);
    DEFINE_OPERATOR(GW_Bool);
    DEFINE_OPERATOR(GW_Float);

    #undef DEFINE_OPERATOR

    /** special treatment for strings */
    GW_Ifstream& operator >> (string& v)
    {
        GW_U32 nSize;
        (*this) >> nSize;
        char* c = new char[nSize+1];
        c[nSize] = NULL;
        this->read( c, (int) nSize );
        v = string(c);
        return *this;
    }
};


//-------------------------------------------------------------------------
/** \name serialization macros */
//-------------------------------------------------------------------------
//@{
/** defines and implement GetClassName and GetClassName for a normal class */
#define GW_DEFINE_SERIALIZATION(class)                                    \
string GetClassName() { return string( #class ); }            \
static GW_Serializable& CreateInstance() { return *(new class); }    \
GW_Serializable* Clone() const { return new class( *this ); }
/** defines and implement GetClassName and GetClassName for a template class */
#define GW_DEFINE_SERIALIZATION_TEMPLATE(class, T)                        \
string GetClassName() { return string( #class ); }            \
static GW_Serializable& CreateInstance() { return *(new class<T>); }\
GW_Serializable* Clone() const { return (new class<T>( *this ) ); }
//@}

typedef class GW_Serializable& (*T_CreateInstance_Function)(void);

/*------------------------------------------------------------------------------*/
/**
 *  \class  GW_Serializable
 *  \brief  Base class for all class that want to be serialized into a file.
 *  \author Gabriel Peyré
 *  \date   4-2-2003
 *
 *  You must overload \c BuildFromFile and \c BuildToFile.
 *    You also must put somewhere in the class header :
 *    \c GW_DEFINE_SERIALIZATION(you_class_name);
 */
/*------------------------------------------------------------------------------*/

class GW_Serializable
{
public:

    /** import the data from an opened file to the object */
    virtual void BuildFromFile( GW_Ifstream& file)
    {
        // nothing
    }
    /** export the data from the object to an opened file */
    virtual void BuildToFile( GW_Ofstream& file )
    {
        // nothing
    }
    /** get the name of the class, useful for the class factory */
    virtual string GetClassName()
    {
        return string("undefined");
    }

    /** duplicate the class : it's a virtual copy constructor */
    virtual GW_Serializable* Clone() const
    {
        return NULL;
    }

    void TestClass()
    {
        GW_U32 nTest = 33;
        GW_Float rTest = (GW_Float) 3.14;
        string sTest = "Youhou, c'est cool !";

        GW_Ofstream file( GW_TEST_FILE );
        file << nTest;
        file << rTest;
        file << sTest;
        file.close();

        GW_U32 nTest1;
        GW_Float rTest1;
        string sTest1;

        GW_Ifstream file2( GW_TEST_FILE );
        file2 >> nTest1;
        file2 >> rTest1;
        file2 >> sTest1;
        file2.close();

        GW_ASSERT( nTest==nTest1 );
        GW_ASSERT( rTest==rTest1 );
        GW_ASSERT( sTest==sTest1 );
}

};

//-------------------------------------------------------------------------
/** \name operators on OR_Serializable */
//-------------------------------------------------------------------------
//@{
inline
GW_Ofstream& operator << (GW_Ofstream& file, GW_Serializable& v)
{
    if( !file.is_open() )
    {
        std::cerr << "File is not opened.";
        return file;
    }

    v.BuildToFile(file);
    return file;
}

inline
GW_Ifstream& operator >> (GW_Ifstream& file, GW_Serializable& v)
{
    if( !file.is_open() )
    {
        std::cerr << "File is not opened.";
        return file;
    }

    v.BuildFromFile(file);
    return file;
}

//@}

} // End namespace GW



#endif // _GW_SERIALIZABLE_H_


///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) Gabriel Peyré
///////////////////////////////////////////////////////////////////////////////
//                               END OF FILE                                 //
///////////////////////////////////////////////////////////////////////////////
