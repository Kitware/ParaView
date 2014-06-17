
#include "pqTextEdit.h"

#include "QTestApp.h"

#include <QDebug>
#include <QSignalSpy>
#include <QSpinBox>

namespace
{
bool testTextEdit(pqTextEdit& textEdit,
                  QSignalSpy& spy, int numberOfSignals, QString text)
{
  bool success = (spy.count() == numberOfSignals);
  success &= (textEdit.toPlainText() == text);

  if (!success)
    {
    qDebug()<<"Signal count: ";
    qDebug()<<"  Expected: "<<numberOfSignals<<" got: "<<spy.count();
    qDebug()<<"toPlainText: ";
    qDebug()<<"  Expected: "<<text<<" got: "<<textEdit.toPlainText();
    }

  spy.clear();
  return success;
}

enum SIGNAL_TYPE
{
  Changed = 0,
  EditingFinished,
  EditingFinishedAndChanged,
};

bool test(pqTextEdit& textEdit,
          QSignalSpy& spy,
          SIGNAL_TYPE signal,
          Qt::Key focusOutKey,
          Qt::KeyboardModifiers focusOutMod)
{
  bool success = true;

  int numberOfChangedSignals = 0;
  int numberOfEditingFinishedAndChangedSignals = 0;
  int numberOfEditingFinishedSignals = 0;
  int numberOfClearedSignals = 0;

  QString standard = "text";
  QString weirUpperCases = "WiERdUppERCase";
  QString additionText = "andsome";
  int numberOfLettersReTyped = 3;

  if (signal == Changed)
    {
    numberOfChangedSignals = (standard + weirUpperCases).length();
    // 3*numberOfLettersReTyped because once for the backspaces, once for the
    // text changed once the last letter is removed and once for the new letter
    // added after that.
    numberOfEditingFinishedAndChangedSignals =  additionText.length();
    numberOfEditingFinishedSignals = 3*numberOfLettersReTyped;
    numberOfClearedSignals = 1;
    }
  else if (signal == EditingFinished)
    {
    numberOfChangedSignals = 0;
    numberOfEditingFinishedAndChangedSignals = 1;
    numberOfEditingFinishedSignals = 1;
    numberOfClearedSignals = 0;
    }
  else if (signal == EditingFinishedAndChanged)
    {
    numberOfChangedSignals = 0;
    numberOfEditingFinishedAndChangedSignals = 1;
    numberOfEditingFinishedSignals = 1; // See pqTextEdit doc
    numberOfClearedSignals = 0;
    }
  else
    {
    qCritical()<<"Developer error: Check your signal !";
    }

  textEdit.setFocus();
  QTestApp::keyClicks(&textEdit, standard);
  QTestApp::keyClicks(&textEdit, weirUpperCases);
  success &= testTextEdit(textEdit, spy,
    numberOfChangedSignals,
    standard + weirUpperCases);
  success &= textEdit.hasFocus();

  textEdit.setFocus();
  QTestApp::keyClicks(&textEdit, additionText);
  QTestApp::keyClick(&textEdit, focusOutKey, focusOutMod);
  success &= testTextEdit(textEdit, spy,
    numberOfEditingFinishedAndChangedSignals,
    standard + weirUpperCases + additionText);
  success &= !textEdit.hasFocus();

  textEdit.setFocus();
  for (int i = 0; i < numberOfLettersReTyped; ++i)
    {
    QTestApp::keyClick(&textEdit, Qt::Key_Backspace);
    }
  QString retyped = (standard + weirUpperCases + additionText).right(numberOfLettersReTyped);
  QTestApp::keyClicks(&textEdit, retyped);
  QTestApp::keyClick(&textEdit, focusOutKey, focusOutMod);

  success &= testTextEdit(textEdit, spy,
    numberOfEditingFinishedSignals,
    standard + weirUpperCases + additionText);
  success &= !textEdit.hasFocus();

  textEdit.clear();
  success &= testTextEdit(textEdit, spy, numberOfClearedSignals, "");

  textEdit.clearFocus();
  return success;
}

} // end namespace

int pqTextEditTest(int argc, char* argv[])
{
  QTestApp app(argc, argv);
  bool success = false;
  pqTextEdit textEdit;
  textEdit.show();

  //QSpinBox spinBox;
  //QObject::connect(&textEdit, SIGNAL(textChanged()), &spinBox, SLOT(stepUp()));
  //spinBox.show();

  //
  // Test text changed
  //
  QSignalSpy changedSpy(&textEdit, SIGNAL(textChanged()));

  // 1- setText
  textEdit.setText("New Text");
  success = testTextEdit(textEdit, changedSpy, 1, "New Text");

  textEdit.setText("New Text");
  success &= testTextEdit(textEdit, changedSpy, 1, "New Text");

  textEdit.setText("");
  success &= testTextEdit(textEdit, changedSpy, 1, "");

  // Qt::ControlModifier & Qt::Key_Enter
  success &= test(textEdit, changedSpy, Changed, Qt::Key_Enter, Qt::ControlModifier);

  // Qt::ControlModifier & Qt::Key_Return
  success &= test(textEdit, changedSpy, Changed, Qt::Key_Return, Qt::ControlModifier);

  // Qt::AltModifier & Qt::Key_Enter
  success &= test(textEdit, changedSpy, Changed, Qt::Key_Enter, Qt::AltModifier);

  // Qt::AltModifier & Qt::Key_Return
  success &= test(textEdit, changedSpy, Changed, Qt::Key_Return, Qt::AltModifier);

  if (!success)
    {
    qCritical()<<"Failed with changed event";
    return EXIT_FAILURE;
    }

  //
  // Test editing finished event
  //
  QSignalSpy editingFinishedSpy(&textEdit, SIGNAL(editingFinished()));

  // 1- setText
  // No editingFinished on set...() -> See pqTextEdit doc
  textEdit.setText("New Text");
  success &= testTextEdit(textEdit, editingFinishedSpy, 0, "New Text");
  textEdit.setText("New Text");
  success &= testTextEdit(textEdit, editingFinishedSpy, 0, "New Text");
  textEdit.setText("");
  success &= testTextEdit(textEdit, editingFinishedSpy, 0, "");

  // Qt::ControlModifier & Qt::Key_Enter
  success &= test(textEdit, editingFinishedSpy, EditingFinished, Qt::Key_Enter, Qt::ControlModifier);

  // Qt::Key_Control & Qt::Key_Return
  success &= test(textEdit, editingFinishedSpy, EditingFinished, Qt::Key_Return, Qt::ControlModifier);

  // Qt::AltModifier & Qt::Key_Enter
  success &= test(textEdit, editingFinishedSpy, EditingFinished, Qt::Key_Enter, Qt::AltModifier);

  // Qt::AltModifier & Qt::Key_Return
  success &= test(textEdit, editingFinishedSpy, EditingFinished, Qt::Key_Return, Qt::AltModifier);

  if (!success)
    {
    qCritical()<<"Failed with editing finished event";
    return EXIT_FAILURE;
    }

  //
  // Test text changed and editing finished event
  //
  QSignalSpy changedAndFinishedSpy(&textEdit, SIGNAL(textChangedAndEditingFinished()));

  // 1- setText
  // No textChangedAndEditingFinished on set...() either -> See pqTextEdit doc
  textEdit.setText("New Text");
  success &= testTextEdit(textEdit, changedAndFinishedSpy, 0, "New Text");
  textEdit.setText("New Text");
  success &= testTextEdit(textEdit, changedAndFinishedSpy, 0, "New Text");
  textEdit.setText("");
  success &= testTextEdit(textEdit, changedAndFinishedSpy, 0, "");

  // Qt::ControlModifier & Qt::Key_Enter
  success &= test(textEdit, changedAndFinishedSpy, EditingFinishedAndChanged, Qt::Key_Enter, Qt::ControlModifier);

  // Qt::ControlModifier & Qt::Key_Return
  success &= test(textEdit, changedAndFinishedSpy, EditingFinishedAndChanged, Qt::Key_Return, Qt::ControlModifier);

  // Qt::AltModifier & Qt::Key_Enter
  success &= test(textEdit, changedAndFinishedSpy, EditingFinishedAndChanged, Qt::Key_Enter, Qt::AltModifier);

  // Qt::AltModifier & Qt::Key_Return
  success &= test(textEdit, changedAndFinishedSpy, EditingFinishedAndChanged, Qt::Key_Return, Qt::AltModifier);

  if (!success)
    {
    qCritical()<<"Failed with changed and editing finished event";
    return EXIT_FAILURE;
    }

  //QTestApp::delay(10000);

  return app.exec();
}
