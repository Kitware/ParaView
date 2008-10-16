
import QtTesting

exceptionsList = [ 'pqClientMainWindow/variableToolbar/QWidgetAction0',
                   'pqClientMainWindow/representationToolbar/QWidgetAction0',
                   'pqClientMainWindow/mainToolBar/QAction1',
                   'pqClientMainWindow/mainToolBar/QAction2',
                   'pqClientMainWindow/currentTimeToolbar/QWidgetAction0',
                   'pqClientMainWindow/currentTimeToolbar/QWidgetAction1',
                   'pqClientMainWindow/currentTimeToolbar/QWidgetAction2',
                   'pqClientMainWindow/actionEditColorMap',
                   'pqClientMainWindow/actionResetRange'
                   ]

# test that all actions in the tool bars are in the menus
missingActionsString = QtTesting.invokeMethod('pqClientMainWindow', 'findToolBarActionsNotInMenus')
missingActions = missingActionsString.split(',')
numMissing = 0
for str in missingActions:
  str = str.strip()
  if str not in exceptionsList:
    numMissing = numMissing + 1
    print 'missing action: ' + str

if numMissing != 0:
  raise ValueError('Some actions are missing from the menu')

