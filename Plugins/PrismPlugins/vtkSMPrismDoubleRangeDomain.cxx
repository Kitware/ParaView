#include "vtkSMPrismDoubleRangeDomain.h"

#include "vtkSMDoubleVectorProperty.h"
#include "vtkObjectFactory.h"

#include <vtkstd/vector>

vtkStandardNewMacro(vtkSMPrismDoubleRangeDomain);

struct vtkSMPrismDoubleRangeDomainInternals
    {
    double Min;
    double Max;
    };

//---------------------------------------------------------------------------
vtkSMPrismDoubleRangeDomain::vtkSMPrismDoubleRangeDomain()
    {
    this->DRInternals = new vtkSMPrismDoubleRangeDomainInternals;
    }

//---------------------------------------------------------------------------
vtkSMPrismDoubleRangeDomain::~vtkSMPrismDoubleRangeDomain()
    {
    delete this->DRInternals;
    }


//---------------------------------------------------------------------------
void vtkSMPrismDoubleRangeDomain::Update(vtkSMProperty* prop)
    {
    vtkSMDoubleVectorProperty* dvp =
        vtkSMDoubleVectorProperty::SafeDownCast(prop);
    if (!dvp)
        {
        return;

        }

    if(dvp->GetNumberOfElements()>=2)
        {
        this->DRInternals->Min=dvp->GetElement(0);
        this->DRInternals->Max=dvp->GetElement(1);
        }
    }
//---------------------------------------------------------------------------
int vtkSMPrismDoubleRangeDomain::SetDefaultValues(vtkSMProperty* prop)
    {
    vtkSMDoubleVectorProperty* dvp =
        vtkSMDoubleVectorProperty::SafeDownCast(prop);
    if (!dvp)
        {
        vtkErrorMacro(
            "vtkSMPrismDoubleRangeDomain only works with vtkSMDoubleVectorProperty.");
        return 0;
        }

    dvp->SetElements2(this->DRInternals->Min,this->DRInternals->Max);

    return 1;



    }
//---------------------------------------------------------------------------
void vtkSMPrismDoubleRangeDomain::PrintSelf(ostream& os, vtkIndent indent)
    {
    this->Superclass::PrintSelf(os, indent);
    }
