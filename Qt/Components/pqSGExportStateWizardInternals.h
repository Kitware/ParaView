#ifndef pqSGExportStateWizardInternals_h
#define pqSGExportStateWizardInternals_h

#include "ui_pqExportStateWizard.h"

class pqSGExportStateWizard::pqInternals : public Ui::ExportStateWizard
{
public:
  std::map<QString, pqPipelineSource*> usedSources;
};

#endif
