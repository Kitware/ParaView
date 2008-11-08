
#include "pqCustomViewModules.h"
#include <pqRenderView.h>
#include <pqTwoDRenderView.h>
#include <pqPlotView.h>
//#include <pqTableView.h>
#include <pqComparativeRenderView.h>
#include <pqSpreadSheetView.h>

pqCustomViewModules::pqCustomViewModules(QObject* o)
  : pqStandardViewModules(o)
{
}

pqCustomViewModules::~pqCustomViewModules()
{
}

QStringList pqCustomViewModules::viewTypes() const
{
  return QStringList() << 
    pqRenderView::renderViewType() << 
    pqTwoDRenderView::twoDRenderViewType() <<
//    pqPlotView::barChartType() << 
//    pqPlotView::XYPlotType() << 
//    pqTableView::tableType() <<
//    pqComparativeRenderView::comparativeRenderViewType() <<
    pqSpreadSheetView::spreadsheetViewType();
}
