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

#include <VolumeAttributes.h>
#include <DataNode.h>
#include <ColorControlPoint.h>
#include <GaussianControlPoint.h>

//
// Enum conversion methods for VolumeAttributes::Renderer
//

static const char *Renderer_strings[] = {
"Splatting", "Texture3D", "RayCasting", 
"RayCastingIntegration"};

vtkstd::string
VolumeAttributes::Renderer_ToString(VolumeAttributes::Renderer t)
{
    int index = int(t);
    if(index < 0 || index >= 4) index = 0;
    return Renderer_strings[index];
}

vtkstd::string
VolumeAttributes::Renderer_ToString(int t)
{
    int index = (t < 0 || t >= 4) ? 0 : t;
    return Renderer_strings[index];
}

bool
VolumeAttributes::Renderer_FromString(const vtkstd::string &s, VolumeAttributes::Renderer &val)
{
    val = VolumeAttributes::Splatting;
    for(int i = 0; i < 4; ++i)
    {
        if(s == Renderer_strings[i])
        {
            val = (Renderer)i;
            return true;
        }
    }
    return false;
}

//
// Enum conversion methods for VolumeAttributes::GradientType
//

static const char *GradientType_strings[] = {
"CenteredDifferences", "SobelOperator"};

vtkstd::string
VolumeAttributes::GradientType_ToString(VolumeAttributes::GradientType t)
{
    int index = int(t);
    if(index < 0 || index >= 2) index = 0;
    return GradientType_strings[index];
}

vtkstd::string
VolumeAttributes::GradientType_ToString(int t)
{
    int index = (t < 0 || t >= 2) ? 0 : t;
    return GradientType_strings[index];
}

bool
VolumeAttributes::GradientType_FromString(const vtkstd::string &s, VolumeAttributes::GradientType &val)
{
    val = VolumeAttributes::CenteredDifferences;
    for(int i = 0; i < 2; ++i)
    {
        if(s == GradientType_strings[i])
        {
            val = (GradientType)i;
            return true;
        }
    }
    return false;
}

//
// Enum conversion methods for VolumeAttributes::Scaling
//

static const char *Scaling_strings[] = {
"Linear", "Log10", "Skew"
};

vtkstd::string
VolumeAttributes::Scaling_ToString(VolumeAttributes::Scaling t)
{
    int index = int(t);
    if(index < 0 || index >= 3) index = 0;
    return Scaling_strings[index];
}

vtkstd::string
VolumeAttributes::Scaling_ToString(int t)
{
    int index = (t < 0 || t >= 3) ? 0 : t;
    return Scaling_strings[index];
}

bool
VolumeAttributes::Scaling_FromString(const vtkstd::string &s, VolumeAttributes::Scaling &val)
{
    val = VolumeAttributes::Linear;
    for(int i = 0; i < 3; ++i)
    {
        if(s == Scaling_strings[i])
        {
            val = (Scaling)i;
            return true;
        }
    }
    return false;
}

//
// Enum conversion methods for VolumeAttributes::SamplingType
//

static const char *SamplingType_strings[] = {
"KernelBased", "Rasterization"};

vtkstd::string
VolumeAttributes::SamplingType_ToString(VolumeAttributes::SamplingType t)
{
    int index = int(t);
    if(index < 0 || index >= 2) index = 0;
    return SamplingType_strings[index];
}

vtkstd::string
VolumeAttributes::SamplingType_ToString(int t)
{
    int index = (t < 0 || t >= 2) ? 0 : t;
    return SamplingType_strings[index];
}

bool
VolumeAttributes::SamplingType_FromString(const vtkstd::string &s, VolumeAttributes::SamplingType &val)
{
    val = VolumeAttributes::KernelBased;
    for(int i = 0; i < 2; ++i)
    {
        if(s == SamplingType_strings[i])
        {
            val = (SamplingType)i;
            return true;
        }
    }
    return false;
}

// Type map format string
const char *VolumeAttributes::TypeMapFormatString = "bbafbaisUbfbfbfbfbiiiiidi";

// ****************************************************************************
// Method: VolumeAttributes::VolumeAttributes
//
// Purpose: 
//   Constructor for the VolumeAttributes class.
//
// Note:       Autogenerated by xml2atts.
//
// Programmer: xml2atts
// Creation:   Tue Mar 13 14:48:06 PST 2007
//
// Modifications:
//   
// ****************************************************************************

VolumeAttributes::VolumeAttributes() : 
    AttributeSubject(VolumeAttributes::TypeMapFormatString),
    opacityVariable("default")
{
    legendFlag = true;
    lightingFlag = true;
    SetDefaultColorControlPoints();
    opacityAttenuation = 1;
    freeformFlag = true;
    resampleTarget = 50000;
    for(int i = 0; i < 256; ++i)
        freeformOpacity[i] = (unsigned char)i;
    useColorVarMin = false;
    colorVarMin = 0;
    useColorVarMax = false;
    colorVarMax = 0;
    useOpacityVarMin = false;
    opacityVarMin = 0;
    useOpacityVarMax = false;
    opacityVarMax = 0;
    smoothData = false;
    samplesPerRay = 500;
    rendererType = Splatting;
    gradientType = SobelOperator;
    num3DSlices = 200;
    scaling = Linear;
    skewFactor = 1;
    sampling = Rasterization;
}

// ****************************************************************************
// Method: VolumeAttributes::VolumeAttributes
//
// Purpose: 
//   Copy constructor for the VolumeAttributes class.
//
// Note:       Autogenerated by xml2atts.
//
// Programmer: xml2atts
// Creation:   Tue Mar 13 14:48:06 PST 2007
//
// Modifications:
//   
// ****************************************************************************

VolumeAttributes::VolumeAttributes(const VolumeAttributes &obj) : 
    AttributeSubject(VolumeAttributes::TypeMapFormatString)
{
    int i;

    legendFlag = obj.legendFlag;
    lightingFlag = obj.lightingFlag;
    colorControlPoints = obj.colorControlPoints;
    opacityAttenuation = obj.opacityAttenuation;
    freeformFlag = obj.freeformFlag;
    opacityControlPoints = obj.opacityControlPoints;
    resampleTarget = obj.resampleTarget;
    opacityVariable = obj.opacityVariable;
    for(i = 0; i < 256; ++i)
        freeformOpacity[i] = obj.freeformOpacity[i];

    useColorVarMin = obj.useColorVarMin;
    colorVarMin = obj.colorVarMin;
    useColorVarMax = obj.useColorVarMax;
    colorVarMax = obj.colorVarMax;
    useOpacityVarMin = obj.useOpacityVarMin;
    opacityVarMin = obj.opacityVarMin;
    useOpacityVarMax = obj.useOpacityVarMax;
    opacityVarMax = obj.opacityVarMax;
    smoothData = obj.smoothData;
    samplesPerRay = obj.samplesPerRay;
    rendererType = obj.rendererType;
    gradientType = obj.gradientType;
    num3DSlices = obj.num3DSlices;
    scaling = obj.scaling;
    skewFactor = obj.skewFactor;
    sampling = obj.sampling;

    SelectAll();
}

// ****************************************************************************
// Method: VolumeAttributes::~VolumeAttributes
//
// Purpose: 
//   Destructor for the VolumeAttributes class.
//
// Note:       Autogenerated by xml2atts.
//
// Programmer: xml2atts
// Creation:   Tue Mar 13 14:48:06 PST 2007
//
// Modifications:
//   
// ****************************************************************************

VolumeAttributes::~VolumeAttributes()
{
    // nothing here
}

// ****************************************************************************
// Method: VolumeAttributes::operator = 
//
// Purpose: 
//   Assignment operator for the VolumeAttributes class.
//
// Note:       Autogenerated by xml2atts.
//
// Programmer: xml2atts
// Creation:   Tue Mar 13 14:48:06 PST 2007
//
// Modifications:
//   
// ****************************************************************************

VolumeAttributes& 
VolumeAttributes::operator = (const VolumeAttributes &obj)
{
    if (this == &obj) return *this;
    int i;

    legendFlag = obj.legendFlag;
    lightingFlag = obj.lightingFlag;
    colorControlPoints = obj.colorControlPoints;
    opacityAttenuation = obj.opacityAttenuation;
    freeformFlag = obj.freeformFlag;
    opacityControlPoints = obj.opacityControlPoints;
    resampleTarget = obj.resampleTarget;
    opacityVariable = obj.opacityVariable;
    for(i = 0; i < 256; ++i)
        freeformOpacity[i] = obj.freeformOpacity[i];

    useColorVarMin = obj.useColorVarMin;
    colorVarMin = obj.colorVarMin;
    useColorVarMax = obj.useColorVarMax;
    colorVarMax = obj.colorVarMax;
    useOpacityVarMin = obj.useOpacityVarMin;
    opacityVarMin = obj.opacityVarMin;
    useOpacityVarMax = obj.useOpacityVarMax;
    opacityVarMax = obj.opacityVarMax;
    smoothData = obj.smoothData;
    samplesPerRay = obj.samplesPerRay;
    rendererType = obj.rendererType;
    gradientType = obj.gradientType;
    num3DSlices = obj.num3DSlices;
    scaling = obj.scaling;
    skewFactor = obj.skewFactor;
    sampling = obj.sampling;

    SelectAll();
    return *this;
}

// ****************************************************************************
// Method: VolumeAttributes::operator == 
//
// Purpose: 
//   Comparison operator == for the VolumeAttributes class.
//
// Note:       Autogenerated by xml2atts.
//
// Programmer: xml2atts
// Creation:   Tue Mar 13 14:48:06 PST 2007
//
// Modifications:
//   
// ****************************************************************************

bool
VolumeAttributes::operator == (const VolumeAttributes &obj) const
{
    int i;

    // Compare the freeformOpacity arrays.
    bool freeformOpacity_equal = true;
    for(i = 0; i < 256 && freeformOpacity_equal; ++i)
        freeformOpacity_equal = (freeformOpacity[i] == obj.freeformOpacity[i]);

    // Create the return value
    return ((legendFlag == obj.legendFlag) &&
            (lightingFlag == obj.lightingFlag) &&
            (colorControlPoints == obj.colorControlPoints) &&
            (opacityAttenuation == obj.opacityAttenuation) &&
            (freeformFlag == obj.freeformFlag) &&
            (opacityControlPoints == obj.opacityControlPoints) &&
            (resampleTarget == obj.resampleTarget) &&
            (opacityVariable == obj.opacityVariable) &&
            freeformOpacity_equal &&
            (useColorVarMin == obj.useColorVarMin) &&
            (colorVarMin == obj.colorVarMin) &&
            (useColorVarMax == obj.useColorVarMax) &&
            (colorVarMax == obj.colorVarMax) &&
            (useOpacityVarMin == obj.useOpacityVarMin) &&
            (opacityVarMin == obj.opacityVarMin) &&
            (useOpacityVarMax == obj.useOpacityVarMax) &&
            (opacityVarMax == obj.opacityVarMax) &&
            (smoothData == obj.smoothData) &&
            (samplesPerRay == obj.samplesPerRay) &&
            (rendererType == obj.rendererType) &&
            (gradientType == obj.gradientType) &&
            (num3DSlices == obj.num3DSlices) &&
            (scaling == obj.scaling) &&
            (skewFactor == obj.skewFactor) &&
            (sampling == obj.sampling));
}

// ****************************************************************************
// Method: VolumeAttributes::operator != 
//
// Purpose: 
//   Comparison operator != for the VolumeAttributes class.
//
// Note:       Autogenerated by xml2atts.
//
// Programmer: xml2atts
// Creation:   Tue Mar 13 14:48:06 PST 2007
//
// Modifications:
//   
// ****************************************************************************

bool
VolumeAttributes::operator != (const VolumeAttributes &obj) const
{
    return !(this->operator == (obj));
}

// ****************************************************************************
// Method: VolumeAttributes::TypeName
//
// Purpose: 
//   Type name method for the VolumeAttributes class.
//
// Note:       Autogenerated by xml2atts.
//
// Programmer: xml2atts
// Creation:   Tue Mar 13 14:48:06 PST 2007
//
// Modifications:
//   
// ****************************************************************************

const vtkstd::string
VolumeAttributes::TypeName() const
{
    return "VolumeAttributes";
}

// ****************************************************************************
// Method: VolumeAttributes::CopyAttributes
//
// Purpose: 
//   CopyAttributes method for the VolumeAttributes class.
//
// Note:       Autogenerated by xml2atts.
//
// Programmer: xml2atts
// Creation:   Tue Mar 13 14:48:06 PST 2007
//
// Modifications:
//   
// ****************************************************************************

bool
VolumeAttributes::CopyAttributes(const AttributeGroup *atts)
{
    if(TypeName() != atts->TypeName())
        return false;

    // Call assignment operator.
    const VolumeAttributes *tmp = (const VolumeAttributes *)atts;
    *this = *tmp;

    return true;
}

// ****************************************************************************
// Method: VolumeAttributes::CreateCompatible
//
// Purpose: 
//   CreateCompatible method for the VolumeAttributes class.
//
// Note:       Autogenerated by xml2atts.
//
// Programmer: xml2atts
// Creation:   Tue Mar 13 14:48:06 PST 2007
//
// Modifications:
//   
// ****************************************************************************

AttributeSubject *
VolumeAttributes::CreateCompatible(const vtkstd::string &tname) const
{
    AttributeSubject *retval = 0;
    if(TypeName() == tname)
        retval = new VolumeAttributes(*this);
    // Other cases could go here too. 

    return retval;
}

// ****************************************************************************
// Method: VolumeAttributes::NewInstance
//
// Purpose: 
//   NewInstance method for the VolumeAttributes class.
//
// Note:       Autogenerated by xml2atts.
//
// Programmer: xml2atts
// Creation:   Tue Mar 13 14:48:06 PST 2007
//
// Modifications:
//   
// ****************************************************************************

AttributeSubject *
VolumeAttributes::NewInstance(bool copy) const
{
    AttributeSubject *retval = 0;
    if(copy)
        retval = new VolumeAttributes(*this);
    else
        retval = new VolumeAttributes;

    return retval;
}

// ****************************************************************************
// Method: VolumeAttributes::SelectAll
//
// Purpose: 
//   Selects all attributes.
//
// Note:       Autogenerated by xml2atts.
//
// Programmer: xml2atts
// Creation:   Tue Mar 13 14:48:06 PST 2007
//
// Modifications:
//   
// ****************************************************************************

void
VolumeAttributes::SelectAll()
{
    Select(0, (void *)&legendFlag);
    Select(1, (void *)&lightingFlag);
    Select(2, (void *)&colorControlPoints);
    Select(3, (void *)&opacityAttenuation);
    Select(4, (void *)&freeformFlag);
    Select(5, (void *)&opacityControlPoints);
    Select(6, (void *)&resampleTarget);
    Select(7, (void *)&opacityVariable);
    Select(8, (void *)freeformOpacity, 256);
    Select(9, (void *)&useColorVarMin);
    Select(10, (void *)&colorVarMin);
    Select(11, (void *)&useColorVarMax);
    Select(12, (void *)&colorVarMax);
    Select(13, (void *)&useOpacityVarMin);
    Select(14, (void *)&opacityVarMin);
    Select(15, (void *)&useOpacityVarMax);
    Select(16, (void *)&opacityVarMax);
    Select(17, (void *)&smoothData);
    Select(18, (void *)&samplesPerRay);
    Select(19, (void *)&rendererType);
    Select(20, (void *)&gradientType);
    Select(21, (void *)&num3DSlices);
    Select(22, (void *)&scaling);
    Select(23, (void *)&skewFactor);
    Select(24, (void *)&sampling);
}

///////////////////////////////////////////////////////////////////////////////
// Persistence methods
///////////////////////////////////////////////////////////////////////////////

// ****************************************************************************
// Method: VolumeAttributes::CreateNode
//
// Purpose: 
//   This method creates a DataNode representation of the object so it can be saved to a config file.
//
// Note:       Autogenerated by xml2atts.
//
// Programmer: xml2atts
// Creation:   Tue Mar 13 14:48:06 PST 2007
//
// Modifications:
//   
// ****************************************************************************

bool
VolumeAttributes::CreateNode(DataNode *parentNode, bool completeSave, bool forceAdd)
{
    if(parentNode == 0)
        return false;

    VolumeAttributes defaultObject;
    bool addToParent = false;
    // Create a node for VolumeAttributes.
    DataNode *node = new DataNode("VolumeAttributes");

    if(completeSave || !FieldsEqual(0, &defaultObject))
    {
        addToParent = true;
        node->AddNode(new DataNode("legendFlag", legendFlag));
    }

    if(completeSave || !FieldsEqual(1, &defaultObject))
    {
        addToParent = true;
        node->AddNode(new DataNode("lightingFlag", lightingFlag));
    }

    if(completeSave || !FieldsEqual(2, &defaultObject))
    {
        DataNode *colorControlPointsNode = new DataNode("colorControlPoints");
        if(colorControlPoints.CreateNode(colorControlPointsNode, completeSave, false))
        {
            addToParent = true;
            node->AddNode(colorControlPointsNode);
        }
        else
            delete colorControlPointsNode;
    }

    if(completeSave || !FieldsEqual(3, &defaultObject))
    {
        addToParent = true;
        node->AddNode(new DataNode("opacityAttenuation", opacityAttenuation));
    }

    if(completeSave || !FieldsEqual(4, &defaultObject))
    {
        addToParent = true;
        node->AddNode(new DataNode("freeformFlag", freeformFlag));
    }

    if(completeSave || !FieldsEqual(5, &defaultObject))
    {
        DataNode *opacityControlPointsNode = new DataNode("opacityControlPoints");
        if(opacityControlPoints.CreateNode(opacityControlPointsNode, completeSave, false))
        {
            addToParent = true;
            node->AddNode(opacityControlPointsNode);
        }
        else
            delete opacityControlPointsNode;
    }

    if(completeSave || !FieldsEqual(6, &defaultObject))
    {
        addToParent = true;
        node->AddNode(new DataNode("resampleTarget", resampleTarget));
    }

    if(completeSave || !FieldsEqual(7, &defaultObject))
    {
        addToParent = true;
        node->AddNode(new DataNode("opacityVariable", opacityVariable));
    }

    if(completeSave || !FieldsEqual(8, &defaultObject))
    {
        addToParent = true;
        node->AddNode(new DataNode("freeformOpacity", freeformOpacity, 256));
    }

    if(completeSave || !FieldsEqual(9, &defaultObject))
    {
        addToParent = true;
        node->AddNode(new DataNode("useColorVarMin", useColorVarMin));
    }

    if(completeSave || !FieldsEqual(10, &defaultObject))
    {
        addToParent = true;
        node->AddNode(new DataNode("colorVarMin", colorVarMin));
    }

    if(completeSave || !FieldsEqual(11, &defaultObject))
    {
        addToParent = true;
        node->AddNode(new DataNode("useColorVarMax", useColorVarMax));
    }

    if(completeSave || !FieldsEqual(12, &defaultObject))
    {
        addToParent = true;
        node->AddNode(new DataNode("colorVarMax", colorVarMax));
    }

    if(completeSave || !FieldsEqual(13, &defaultObject))
    {
        addToParent = true;
        node->AddNode(new DataNode("useOpacityVarMin", useOpacityVarMin));
    }

    if(completeSave || !FieldsEqual(14, &defaultObject))
    {
        addToParent = true;
        node->AddNode(new DataNode("opacityVarMin", opacityVarMin));
    }

    if(completeSave || !FieldsEqual(15, &defaultObject))
    {
        addToParent = true;
        node->AddNode(new DataNode("useOpacityVarMax", useOpacityVarMax));
    }

    if(completeSave || !FieldsEqual(16, &defaultObject))
    {
        addToParent = true;
        node->AddNode(new DataNode("opacityVarMax", opacityVarMax));
    }

    if(completeSave || !FieldsEqual(17, &defaultObject))
    {
        addToParent = true;
        node->AddNode(new DataNode("smoothData", smoothData));
    }

    if(completeSave || !FieldsEqual(18, &defaultObject))
    {
        addToParent = true;
        node->AddNode(new DataNode("samplesPerRay", samplesPerRay));
    }

    if(completeSave || !FieldsEqual(19, &defaultObject))
    {
        addToParent = true;
        node->AddNode(new DataNode("rendererType", Renderer_ToString(rendererType)));
    }

    if(completeSave || !FieldsEqual(20, &defaultObject))
    {
        addToParent = true;
        node->AddNode(new DataNode("gradientType", GradientType_ToString(gradientType)));
    }

    if(completeSave || !FieldsEqual(21, &defaultObject))
    {
        addToParent = true;
        node->AddNode(new DataNode("num3DSlices", num3DSlices));
    }

    if(completeSave || !FieldsEqual(22, &defaultObject))
    {
        addToParent = true;
        node->AddNode(new DataNode("scaling", Scaling_ToString(scaling)));
    }

    if(completeSave || !FieldsEqual(23, &defaultObject))
    {
        addToParent = true;
        node->AddNode(new DataNode("skewFactor", skewFactor));
    }

    if(completeSave || !FieldsEqual(24, &defaultObject))
    {
        addToParent = true;
        node->AddNode(new DataNode("sampling", SamplingType_ToString(sampling)));
    }


    // Add the node to the parent node.
    if(addToParent || forceAdd)
        parentNode->AddNode(node);
    else
        delete node;

    return (addToParent || forceAdd);
}

// ****************************************************************************
// Method: VolumeAttributes::SetFromNode
//
// Purpose: 
//   This method sets attributes in this object from values in a DataNode representation of the object.
//
// Note:       Autogenerated by xml2atts.
//
// Programmer: xml2atts
// Creation:   Tue Mar 13 14:48:06 PST 2007
//
// Modifications:
//   
// ****************************************************************************

void
VolumeAttributes::SetFromNode(DataNode *parentNode)
{
    //int i;
    if(parentNode == 0)
        return;

    DataNode *searchNode = parentNode->GetNode("VolumeAttributes");
    if(searchNode == 0)
        return;

    DataNode *node;
    if((node = searchNode->GetNode("legendFlag")) != 0)
        SetLegendFlag(node->AsBool());
    if((node = searchNode->GetNode("lightingFlag")) != 0)
        SetLightingFlag(node->AsBool());
    if((node = searchNode->GetNode("colorControlPoints")) != 0)
        colorControlPoints.SetFromNode(node);
    if((node = searchNode->GetNode("opacityAttenuation")) != 0)
        SetOpacityAttenuation(node->AsFloat());
    if((node = searchNode->GetNode("freeformFlag")) != 0)
        SetFreeformFlag(node->AsBool());
    if((node = searchNode->GetNode("opacityControlPoints")) != 0)
        opacityControlPoints.SetFromNode(node);
    if((node = searchNode->GetNode("resampleTarget")) != 0)
        SetResampleTarget(node->AsInt());
    if((node = searchNode->GetNode("opacityVariable")) != 0)
        SetOpacityVariable(node->AsString());
    if((node = searchNode->GetNode("freeformOpacity")) != 0)
        SetFreeformOpacity(node->AsUnsignedCharArray());
    if((node = searchNode->GetNode("useColorVarMin")) != 0)
        SetUseColorVarMin(node->AsBool());
    if((node = searchNode->GetNode("colorVarMin")) != 0)
        SetColorVarMin(node->AsFloat());
    if((node = searchNode->GetNode("useColorVarMax")) != 0)
        SetUseColorVarMax(node->AsBool());
    if((node = searchNode->GetNode("colorVarMax")) != 0)
        SetColorVarMax(node->AsFloat());
    if((node = searchNode->GetNode("useOpacityVarMin")) != 0)
        SetUseOpacityVarMin(node->AsBool());
    if((node = searchNode->GetNode("opacityVarMin")) != 0)
        SetOpacityVarMin(node->AsFloat());
    if((node = searchNode->GetNode("useOpacityVarMax")) != 0)
        SetUseOpacityVarMax(node->AsBool());
    if((node = searchNode->GetNode("opacityVarMax")) != 0)
        SetOpacityVarMax(node->AsFloat());
    if((node = searchNode->GetNode("smoothData")) != 0)
        SetSmoothData(node->AsBool());
    if((node = searchNode->GetNode("samplesPerRay")) != 0)
        SetSamplesPerRay(node->AsInt());
    if((node = searchNode->GetNode("rendererType")) != 0)
    {
        // Allow enums to be int or string in the config file
        if(node->GetNodeType() == INT_NODE)
        {
            int ival = node->AsInt();
            if(ival >= 0 && ival < 4)
                SetRendererType(Renderer(ival));
        }
        else if(node->GetNodeType() == STRING_NODE)
        {
            Renderer value;
            if(Renderer_FromString(node->AsString(), value))
                SetRendererType(value);
        }
    }
    if((node = searchNode->GetNode("gradientType")) != 0)
    {
        // Allow enums to be int or string in the config file
        if(node->GetNodeType() == INT_NODE)
        {
            int ival = node->AsInt();
            if(ival >= 0 && ival < 2)
                SetGradientType(GradientType(ival));
        }
        else if(node->GetNodeType() == STRING_NODE)
        {
            GradientType value;
            if(GradientType_FromString(node->AsString(), value))
                SetGradientType(value);
        }
    }
    if((node = searchNode->GetNode("num3DSlices")) != 0)
        SetNum3DSlices(node->AsInt());
    if((node = searchNode->GetNode("scaling")) != 0)
    {
        // Allow enums to be int or string in the config file
        if(node->GetNodeType() == INT_NODE)
        {
            int ival = node->AsInt();
            if(ival >= 0 && ival < 3)
                SetScaling(Scaling(ival));
        }
        else if(node->GetNodeType() == STRING_NODE)
        {
            Scaling value;
            if(Scaling_FromString(node->AsString(), value))
                SetScaling(value);
        }
    }
    if((node = searchNode->GetNode("skewFactor")) != 0)
        SetSkewFactor(node->AsDouble());
    if((node = searchNode->GetNode("sampling")) != 0)
    {
        // Allow enums to be int or string in the config file
        if(node->GetNodeType() == INT_NODE)
        {
            int ival = node->AsInt();
            if(ival >= 0 && ival < 2)
                SetSampling(SamplingType(ival));
        }
        else if(node->GetNodeType() == STRING_NODE)
        {
            SamplingType value;
            if(SamplingType_FromString(node->AsString(), value))
                SetSampling(value);
        }
    }

    if(colorControlPoints.GetNumControlPoints() < 2)
         SetDefaultColorControlPoints();
}

///////////////////////////////////////////////////////////////////////////////
// Set property methods
///////////////////////////////////////////////////////////////////////////////

void
VolumeAttributes::SetLegendFlag(bool legendFlag_)
{
    legendFlag = legendFlag_;
    Select(0, (void *)&legendFlag);
}

void
VolumeAttributes::SetLightingFlag(bool lightingFlag_)
{
    lightingFlag = lightingFlag_;
    Select(1, (void *)&lightingFlag);
}

void
VolumeAttributes::SetColorControlPoints(const ColorControlPointList &colorControlPoints_)
{
    colorControlPoints = colorControlPoints_;
    Select(2, (void *)&colorControlPoints);
}

void
VolumeAttributes::SetOpacityAttenuation(float opacityAttenuation_)
{
    opacityAttenuation = opacityAttenuation_;
    Select(3, (void *)&opacityAttenuation);
}

void
VolumeAttributes::SetFreeformFlag(bool freeformFlag_)
{
    freeformFlag = freeformFlag_;
    Select(4, (void *)&freeformFlag);
}

void
VolumeAttributes::SetOpacityControlPoints(const GaussianControlPointList &opacityControlPoints_)
{
    opacityControlPoints = opacityControlPoints_;
    Select(5, (void *)&opacityControlPoints);
}

void
VolumeAttributes::SetResampleTarget(int resampleTarget_)
{
    resampleTarget = resampleTarget_;
    Select(6, (void *)&resampleTarget);
}

void
VolumeAttributes::SetOpacityVariable(const vtkstd::string &opacityVariable_)
{
    opacityVariable = opacityVariable_;
    Select(7, (void *)&opacityVariable);
}

void
VolumeAttributes::SetFreeformOpacity(const unsigned char *freeformOpacity_)
{
    for(int i = 0; i < 256; ++i)
        freeformOpacity[i] = freeformOpacity_[i];
    Select(8, (void *)freeformOpacity, 256);
}

void
VolumeAttributes::SetUseColorVarMin(bool useColorVarMin_)
{
    useColorVarMin = useColorVarMin_;
    Select(9, (void *)&useColorVarMin);
}

void
VolumeAttributes::SetColorVarMin(float colorVarMin_)
{
    colorVarMin = colorVarMin_;
    Select(10, (void *)&colorVarMin);
}

void
VolumeAttributes::SetUseColorVarMax(bool useColorVarMax_)
{
    useColorVarMax = useColorVarMax_;
    Select(11, (void *)&useColorVarMax);
}

void
VolumeAttributes::SetColorVarMax(float colorVarMax_)
{
    colorVarMax = colorVarMax_;
    Select(12, (void *)&colorVarMax);
}

void
VolumeAttributes::SetUseOpacityVarMin(bool useOpacityVarMin_)
{
    useOpacityVarMin = useOpacityVarMin_;
    Select(13, (void *)&useOpacityVarMin);
}

void
VolumeAttributes::SetOpacityVarMin(float opacityVarMin_)
{
    opacityVarMin = opacityVarMin_;
    Select(14, (void *)&opacityVarMin);
}

void
VolumeAttributes::SetUseOpacityVarMax(bool useOpacityVarMax_)
{
    useOpacityVarMax = useOpacityVarMax_;
    Select(15, (void *)&useOpacityVarMax);
}

void
VolumeAttributes::SetOpacityVarMax(float opacityVarMax_)
{
    opacityVarMax = opacityVarMax_;
    Select(16, (void *)&opacityVarMax);
}

void
VolumeAttributes::SetSmoothData(bool smoothData_)
{
    smoothData = smoothData_;
    Select(17, (void *)&smoothData);
}

void
VolumeAttributes::SetSamplesPerRay(int samplesPerRay_)
{
    samplesPerRay = samplesPerRay_;
    Select(18, (void *)&samplesPerRay);
}

void
VolumeAttributes::SetRendererType(VolumeAttributes::Renderer rendererType_)
{
    rendererType = rendererType_;
    Select(19, (void *)&rendererType);
}

void
VolumeAttributes::SetGradientType(VolumeAttributes::GradientType gradientType_)
{
    gradientType = gradientType_;
    Select(20, (void *)&gradientType);
}

void
VolumeAttributes::SetNum3DSlices(int num3DSlices_)
{
    num3DSlices = num3DSlices_;
    Select(21, (void *)&num3DSlices);
}

void
VolumeAttributes::SetScaling(VolumeAttributes::Scaling scaling_)
{
    scaling = scaling_;
    Select(22, (void *)&scaling);
}

void
VolumeAttributes::SetSkewFactor(double skewFactor_)
{
    skewFactor = skewFactor_;
    Select(23, (void *)&skewFactor);
}

void
VolumeAttributes::SetSampling(VolumeAttributes::SamplingType sampling_)
{
    sampling = sampling_;
    Select(24, (void *)&sampling);
}

///////////////////////////////////////////////////////////////////////////////
// Get property methods
///////////////////////////////////////////////////////////////////////////////

bool
VolumeAttributes::GetLegendFlag() const
{
    return legendFlag;
}

bool
VolumeAttributes::GetLightingFlag() const
{
    return lightingFlag;
}

const ColorControlPointList &
VolumeAttributes::GetColorControlPoints() const
{
    return colorControlPoints;
}

ColorControlPointList &
VolumeAttributes::GetColorControlPoints()
{
    return colorControlPoints;
}

float
VolumeAttributes::GetOpacityAttenuation() const
{
    return opacityAttenuation;
}

bool
VolumeAttributes::GetFreeformFlag() const
{
    return freeformFlag;
}

const GaussianControlPointList &
VolumeAttributes::GetOpacityControlPoints() const
{
    return opacityControlPoints;
}

GaussianControlPointList &
VolumeAttributes::GetOpacityControlPoints()
{
    return opacityControlPoints;
}

int
VolumeAttributes::GetResampleTarget() const
{
    return resampleTarget;
}

const vtkstd::string &
VolumeAttributes::GetOpacityVariable() const
{
    return opacityVariable;
}

vtkstd::string &
VolumeAttributes::GetOpacityVariable()
{
    return opacityVariable;
}

const unsigned char *
VolumeAttributes::GetFreeformOpacity() const
{
    return freeformOpacity;
}

unsigned char *
VolumeAttributes::GetFreeformOpacity()
{
    return freeformOpacity;
}

bool
VolumeAttributes::GetUseColorVarMin() const
{
    return useColorVarMin;
}

float
VolumeAttributes::GetColorVarMin() const
{
    return colorVarMin;
}

bool
VolumeAttributes::GetUseColorVarMax() const
{
    return useColorVarMax;
}

float
VolumeAttributes::GetColorVarMax() const
{
    return colorVarMax;
}

bool
VolumeAttributes::GetUseOpacityVarMin() const
{
    return useOpacityVarMin;
}

float
VolumeAttributes::GetOpacityVarMin() const
{
    return opacityVarMin;
}

bool
VolumeAttributes::GetUseOpacityVarMax() const
{
    return useOpacityVarMax;
}

float
VolumeAttributes::GetOpacityVarMax() const
{
    return opacityVarMax;
}

bool
VolumeAttributes::GetSmoothData() const
{
    return smoothData;
}

int
VolumeAttributes::GetSamplesPerRay() const
{
    return samplesPerRay;
}

VolumeAttributes::Renderer
VolumeAttributes::GetRendererType() const
{
    return Renderer(rendererType);
}

VolumeAttributes::GradientType
VolumeAttributes::GetGradientType() const
{
    return GradientType(gradientType);
}

int
VolumeAttributes::GetNum3DSlices() const
{
    return num3DSlices;
}

VolumeAttributes::Scaling
VolumeAttributes::GetScaling() const
{
    return Scaling(scaling);
}

double
VolumeAttributes::GetSkewFactor() const
{
    return skewFactor;
}

VolumeAttributes::SamplingType
VolumeAttributes::GetSampling() const
{
    return SamplingType(sampling);
}

///////////////////////////////////////////////////////////////////////////////
// Select property methods
///////////////////////////////////////////////////////////////////////////////

void
VolumeAttributes::SelectColorControlPoints()
{
    Select(2, (void *)&colorControlPoints);
}

void
VolumeAttributes::SelectOpacityControlPoints()
{
    Select(5, (void *)&opacityControlPoints);
}

void
VolumeAttributes::SelectOpacityVariable()
{
    Select(7, (void *)&opacityVariable);
}

void
VolumeAttributes::SelectFreeformOpacity()
{
    Select(8, (void *)freeformOpacity, 256);
}

///////////////////////////////////////////////////////////////////////////////
// Keyframing methods
///////////////////////////////////////////////////////////////////////////////

// ****************************************************************************
// Method: VolumeAttributes::GetFieldName
//
// Purpose: 
//   This method returns the name of a field given its index.
//
// Note:       Autogenerated by xml2atts.
//
// Programmer: xml2atts
// Creation:   Tue Mar 13 14:48:06 PST 2007
//
// Modifications:
//   
// ****************************************************************************

vtkstd::string
VolumeAttributes::GetFieldName(int index) const
{
    switch (index)
    {
        case 0:  return "legendFlag";
        case 1:  return "lightingFlag";
        case 2:  return "colorControlPoints";
        case 3:  return "opacityAttenuation";
        case 4:  return "freeformFlag";
        case 5:  return "opacityControlPoints";
        case 6:  return "resampleTarget";
        case 7:  return "opacityVariable";
        case 8:  return "freeformOpacity";
        case 9:  return "useColorVarMin";
        case 10:  return "colorVarMin";
        case 11:  return "useColorVarMax";
        case 12:  return "colorVarMax";
        case 13:  return "useOpacityVarMin";
        case 14:  return "opacityVarMin";
        case 15:  return "useOpacityVarMax";
        case 16:  return "opacityVarMax";
        case 17:  return "smoothData";
        case 18:  return "samplesPerRay";
        case 19:  return "Renderer Type";
        case 20:  return "Gradient Type";
        case 21:  return "num3DSlices";
        case 22:  return "scaling";
        case 23:  return "skewFactor";
        case 24:  return "Sampling Type";
        default:  return "invalid index";
    }
}

// ****************************************************************************
// Method: VolumeAttributes::GetFieldType
//
// Purpose: 
//   This method returns the type of a field given its index.
//
// Note:       Autogenerated by xml2atts.
//
// Programmer: xml2atts
// Creation:   Tue Mar 13 14:48:06 PST 2007
//
// Modifications:
//   
// ****************************************************************************

AttributeGroup::FieldType
VolumeAttributes::GetFieldType(int index) const
{
    switch (index)
    {
        case 0:  return FieldType_bool;
        case 1:  return FieldType_bool;
        case 2:  return FieldType_att;
        case 3:  return FieldType_float;
        case 4:  return FieldType_bool;
        case 5:  return FieldType_att;
        case 6:  return FieldType_int;
        case 7:  return FieldType_variablename;
        case 8:  return FieldType_ucharArray;
        case 9:  return FieldType_bool;
        case 10:  return FieldType_float;
        case 11:  return FieldType_bool;
        case 12:  return FieldType_float;
        case 13:  return FieldType_bool;
        case 14:  return FieldType_float;
        case 15:  return FieldType_bool;
        case 16:  return FieldType_float;
        case 17:  return FieldType_bool;
        case 18:  return FieldType_int;
        case 19:  return FieldType_enum;
        case 20:  return FieldType_enum;
        case 21:  return FieldType_int;
        case 22:  return FieldType_enum;
        case 23:  return FieldType_double;
        case 24:  return FieldType_enum;
        default:  return FieldType_unknown;
    }
}

// ****************************************************************************
// Method: VolumeAttributes::GetFieldTypeName
//
// Purpose: 
//   This method returns the name of a field type given its index.
//
// Note:       Autogenerated by xml2atts.
//
// Programmer: xml2atts
// Creation:   Tue Mar 13 14:48:06 PST 2007
//
// Modifications:
//   
// ****************************************************************************

vtkstd::string
VolumeAttributes::GetFieldTypeName(int index) const
{
    switch (index)
    {
        case 0:  return "bool";
        case 1:  return "bool";
        case 2:  return "att";
        case 3:  return "float";
        case 4:  return "bool";
        case 5:  return "att";
        case 6:  return "int";
        case 7:  return "variablename";
        case 8:  return "ucharArray";
        case 9:  return "bool";
        case 10:  return "float";
        case 11:  return "bool";
        case 12:  return "float";
        case 13:  return "bool";
        case 14:  return "float";
        case 15:  return "bool";
        case 16:  return "float";
        case 17:  return "bool";
        case 18:  return "int";
        case 19:  return "enum";
        case 20:  return "enum";
        case 21:  return "int";
        case 22:  return "enum";
        case 23:  return "double";
        case 24:  return "enum";
        default:  return "invalid index";
    }
}

// ****************************************************************************
// Method: VolumeAttributes::FieldsEqual
//
// Purpose: 
//   This method compares two fields and return true if they are equal.
//
// Note:       Autogenerated by xml2atts.
//
// Programmer: xml2atts
// Creation:   Tue Mar 13 14:48:06 PST 2007
//
// Modifications:
//   
// ****************************************************************************

bool
VolumeAttributes::FieldsEqual(int index_, const AttributeGroup *rhs) const
{
    int i;

    const VolumeAttributes &obj = *((const VolumeAttributes*)rhs);
    bool retval = false;
    switch (index_)
    {
    case 0:
        {  // new scope
        retval = (legendFlag == obj.legendFlag);
        }
        break;
    case 1:
        {  // new scope
        retval = (lightingFlag == obj.lightingFlag);
        }
        break;
    case 2:
        {  // new scope
        retval = (colorControlPoints == obj.colorControlPoints);
        }
        break;
    case 3:
        {  // new scope
        retval = (opacityAttenuation == obj.opacityAttenuation);
        }
        break;
    case 4:
        {  // new scope
        retval = (freeformFlag == obj.freeformFlag);
        }
        break;
    case 5:
        {  // new scope
        retval = (opacityControlPoints == obj.opacityControlPoints);
        }
        break;
    case 6:
        {  // new scope
        retval = (resampleTarget == obj.resampleTarget);
        }
        break;
    case 7:
        {  // new scope
        retval = (opacityVariable == obj.opacityVariable);
        }
        break;
    case 8:
        {  // new scope
        // Compare the freeformOpacity arrays.
        bool freeformOpacity_equal = true;
        for(i = 0; i < 256 && freeformOpacity_equal; ++i)
            freeformOpacity_equal = (freeformOpacity[i] == obj.freeformOpacity[i]);

        retval = freeformOpacity_equal;
        }
        break;
    case 9:
        {  // new scope
        retval = (useColorVarMin == obj.useColorVarMin);
        }
        break;
    case 10:
        {  // new scope
        retval = (colorVarMin == obj.colorVarMin);
        }
        break;
    case 11:
        {  // new scope
        retval = (useColorVarMax == obj.useColorVarMax);
        }
        break;
    case 12:
        {  // new scope
        retval = (colorVarMax == obj.colorVarMax);
        }
        break;
    case 13:
        {  // new scope
        retval = (useOpacityVarMin == obj.useOpacityVarMin);
        }
        break;
    case 14:
        {  // new scope
        retval = (opacityVarMin == obj.opacityVarMin);
        }
        break;
    case 15:
        {  // new scope
        retval = (useOpacityVarMax == obj.useOpacityVarMax);
        }
        break;
    case 16:
        {  // new scope
        retval = (opacityVarMax == obj.opacityVarMax);
        }
        break;
    case 17:
        {  // new scope
        retval = (smoothData == obj.smoothData);
        }
        break;
    case 18:
        {  // new scope
        retval = (samplesPerRay == obj.samplesPerRay);
        }
        break;
    case 19:
        {  // new scope
        retval = (rendererType == obj.rendererType);
        }
        break;
    case 20:
        {  // new scope
        retval = (gradientType == obj.gradientType);
        }
        break;
    case 21:
        {  // new scope
        retval = (num3DSlices == obj.num3DSlices);
        }
        break;
    case 22:
        {  // new scope
        retval = (scaling == obj.scaling);
        }
        break;
    case 23:
        {  // new scope
        retval = (skewFactor == obj.skewFactor);
        }
        break;
    case 24:
        {  // new scope
        retval = (sampling == obj.sampling);
        }
        break;
    default: retval = false;
    }

    return retval;
}

///////////////////////////////////////////////////////////////////////////////
// User-defined methods.
///////////////////////////////////////////////////////////////////////////////

// ****************************************************************************
//  Method:  VolumeAttributes::ChangesRequireRecalculation
//
//  Modifications:
//    Jeremy Meredith, Thu Oct  2 13:27:54 PDT 2003
//    Let changes in rendererType to force a recalculation.  This is 
//    appropriate since the 3D texturing renderer prefers different 
//    dimensions (i.e. powers of two) than the splatting renderer.
//
//    Hank Childs, Mon Dec 15 14:42:26 PST 2003
//    Recalculate if the smooth option was hit.
//
//    Hank Childs, Mon Nov 22 09:37:12 PST 2004
//    Recalculate if the ray trace button was hit.
//
//    Brad Whitlock, Wed Dec 15 09:31:24 PDT 2004
//    Removed doSoftware since it's now part of rendererType.
//
//    Kathleen Bonnell, Thu Mar  3 09:27:40 PST 2005
//    Recalculate if scaling or skewFactor changed for RayCasting.
//
// ****************************************************************************
bool
VolumeAttributes::ChangesRequireRecalculation(const VolumeAttributes &obj) const
{
    if (opacityVariable != obj.opacityVariable)
        return true;
    if (resampleTarget != obj.resampleTarget)
        return true;
    if (rendererType != obj.rendererType)
        return true;
    if (smoothData != obj.smoothData)
        return true;

    if (rendererType == VolumeAttributes::RayCasting)
    {
        if (scaling != obj.scaling)
            return true;
        if (scaling == VolumeAttributes::Skew && skewFactor != obj.skewFactor)
            return true;
    }

    return false;
}

// ****************************************************************************
//  Method:  VolumeAttributes::GradientWontChange
//
//  Purpose:
//    Determines if the gradient can avoid being invalidated.
//
//  Arguments:
//    obj        the attributes to compare with
//
//  Programmer:  Jeremy Meredith
//  Creation:    September 30, 2003
//
//  Modifications:
//    Jeremy Meredith, Thu Oct  2 13:30:29 PDT 2003
//    Added rendererType and gradientType to the list of modifications
//    that require re-calculating the gradient.
//
// ****************************************************************************
bool
VolumeAttributes::GradientWontChange(const VolumeAttributes &obj) const
{
    int i;

    // Compare the freeformOpacity arrays.
    bool freeformOpacity_equal = true;
    for(i = 0; i < 256 && freeformOpacity_equal; ++i)
        freeformOpacity_equal = (freeformOpacity[i] == obj.freeformOpacity[i]);

    // Create the return value
    return ((freeformFlag         == obj.freeformFlag) &&
            (opacityControlPoints == obj.opacityControlPoints) &&
            (resampleTarget       == obj.resampleTarget) &&
            (opacityVariable      == obj.opacityVariable) &&
            freeformOpacity_equal &&
            (useColorVarMin       == obj.useColorVarMin) &&
            (colorVarMin          == obj.colorVarMin) &&
            (useColorVarMax       == obj.useColorVarMax) &&
            (colorVarMax          == obj.colorVarMax) &&
            (useOpacityVarMin     == obj.useOpacityVarMin) &&
            (opacityVarMin        == obj.opacityVarMin) &&
            (useOpacityVarMax     == obj.useOpacityVarMax) &&
            (opacityVarMax        == obj.opacityVarMax) &&
            (rendererType         == obj.rendererType) &&
            (gradientType         == obj.gradientType));
}


// ****************************************************************************
// Method: VolumeAttributes::GetTransferFunction
//
// Purpose: 
//   This method calculates the transfer function and stores it in the rgba
//   array that is passed in.
//
// Programmer: Brad Whitlock
// Creation:   Tue Aug 21 15:44:34 PST 2001
//
// Modifications:
//   Brad Whitlock, Thu Nov 21 15:05:25 PST 2002
//   GetColors has been moved to ColorControlPointList. I updated this code
//   to take that into account.
//
//   Jeremy Meredith, Thu Oct  2 13:29:40 PDT 2003
//   Made the method const.
//
// ****************************************************************************

void
VolumeAttributes::GetTransferFunction(unsigned char *rgba) const
{
    unsigned char rgb[256 * 3];
    unsigned char alphas[256];
    const unsigned char *a_ptr;

    // Figure out the colors
    colorControlPoints.GetColors(rgb, 256);
    // Figure out the opacities
    if(freeformFlag)
        a_ptr = freeformOpacity;
    else
    {
        GetGaussianOpacities(alphas);
        a_ptr = alphas;
    }

    unsigned char *rgb_ptr = rgb;
    unsigned char *rgba_ptr = rgba;
    for(int i = 0; i < 256; ++i)
    {
        // Copy the color
        *rgba_ptr++ = *rgb_ptr++;
        *rgba_ptr++ = *rgb_ptr++;
        *rgba_ptr++ = *rgb_ptr++;
        // Copy the alpha
        *rgba_ptr++ = *a_ptr++;
    }
}

// ****************************************************************************
// Method: VolumeAttributes::SetDefaultColorControlPoints
//
// Purpose: 
//   This method replaces all of the color control points in the list with the
//   default color control points.
//
// Programmer: Brad Whitlock
// Creation:   Tue Aug 21 15:44:34 PST 2001
//
// Modifications:
//   
// ****************************************************************************

void
VolumeAttributes::SetDefaultColorControlPoints()
{
    const float positions[] = {0., 0.25, 0.5, 0.75, 1.};
    const unsigned char colors[5][4] = {
        {0,   0,   255, 255},
        {0,   255, 255, 255},
        {0,   255, 0,   255},
        {255, 255, 0,   255},
        {255, 0,   0,   255}};

    // Clear the color control point list.
    colorControlPoints.ClearControlPoints();

    // Set the default control points in the color control point list.
    for(int i = 0; i < 5; ++i)
    {
        ColorControlPoint cpt;
        cpt.SetPosition(positions[i]);
        cpt.SetColors(colors[i]);
        colorControlPoints.AddControlPoints(cpt);
    }
    SelectColorControlPoints();
}

// ****************************************************************************
// Method: VolumeAttributes::GetGaussianOpacities
//
// Purpose: 
//   This method calculates the opacities using the object's gaussian control
//   point list and stores the results in the alphas array that is passed in.
//
// Arguments:
//   alphas : The return array for the colors.
//
// Programmer: Brad Whitlock
// Creation:   Thu Sep 6 10:23:59 PDT 2001
//
// Modifications:
//    Jeremy Meredith, Thu Oct  2 13:30:00 PDT 2003
//    Made the method const.
//
// ****************************************************************************
#include <math.h>
void
VolumeAttributes::GetGaussianOpacities(unsigned char *alphas) const
{
    int i;
    float values[256];
    for (i=0; i<256; i++)
        values[i] = 0.;

    for (int p=0; p<opacityControlPoints.GetNumControlPoints(); p++)
    {
        const GaussianControlPoint &pt =
                               opacityControlPoints.GetControlPoints(p);
        float pos    = pt.GetX();
        float width  = pt.GetWidth();
        float height = pt.GetHeight();
        float xbias  = pt.GetXBias();
        float ybias  = pt.GetYBias();
        for (int i=0; i<256; i++)
        {
            float x = float(i)/float(256-1);

            // clamp non-zero values to pos +/- width
            if (x > pos+width || x < pos-width)
            {
                values[i] = (values[i] > 0.) ? values[i] : 0.;
                continue;
            }

            // non-zero width
            if (width == 0)
                width = .00001;

            // translate the original x to a new x based on the xbias
            float x0;
            if (xbias==0 || x == pos+xbias)
            {
                x0 = x;
            }
            else if (x > pos+xbias)
            {
                if (width == xbias)
                    x0 = pos;
                else
                    x0 = pos+(x-pos-xbias)*(width/(width-xbias));
            }
            else // (x < pos+xbias)
            {
                if (-width == xbias)
                    x0 = pos;
                else
                    x0 = pos-(x-pos-xbias)*(width/(width+xbias));
            }

            // center around 0 and normalize to -1,1
            float x1 = (x0-pos)/width;

            // do a linear interpolation between:
            //    a gaussian and a parabola        if 0<ybias<1
            //    a parabola and a step function   if 1<ybias<2
            float h0a = exp(-(4*x1*x1));
            float h0b = 1. - x1*x1;
            float h0c = 1.;
            float h1;
            if (ybias < 1)
                h1 = ybias*h0b + (1-ybias)*h0a;
            else
                h1 = (2-ybias)*h0b + (ybias-1)*h0c;
            float h2 = height * h1;
            
            // perform the MAX over different guassians, not the sum
            values[i] = (values[i] > h2) ? values[i] : h2;
        }
    }

    // Convert to unsigned char and return.
    for(i = 0; i < 256; ++i)
    {
        int tmp = int(values[i] * 255.);
        if(tmp < 0)
            tmp = 0;
        else if(tmp > 255)
            tmp = 255;
        alphas[i] = (unsigned char)(tmp);
    }
}

void
VolumeAttributes::GetOpacities(unsigned char *alphas)
{
    if(freeformFlag)
    {
        for(int i = 0; i < 256; ++i)
            alphas[i] = freeformOpacity[i];
    }
    else
        GetGaussianOpacities(alphas);
}

void
VolumeAttributes::SetSmoothingFlag(bool val)
{
    colorControlPoints.SetSmoothingFlag(val);
    Select(2, (void *)&colorControlPoints);
}

bool
VolumeAttributes::GetSmoothingFlag() const
{
    return colorControlPoints.GetSmoothingFlag();
}

void
VolumeAttributes::SetEqualSpacingFlag(bool val)
{
    colorControlPoints.SetEqualSpacingFlag(val);
    Select(2, (void *)&colorControlPoints);
}

bool
VolumeAttributes::GetEqualSpacingFlag() const
{
    return colorControlPoints.GetEqualSpacingFlag();
}

