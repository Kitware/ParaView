
#include "pqTextEditTest.h"

#include <QDebug>
#include <QObject>
#include <QSignalSpy>
#include <QTest>

#include "pqTextEdit.h"

// ----------------------------------------------------------------------------
void pqTextEditTester::init()
{
  this->TextEdit = new pqTextEdit();
  this->TextChangedSpy = new QSignalSpy(this->TextEdit, SIGNAL(textChanged()));
  this->EditingFinishedSpy = new QSignalSpy(this->TextEdit, SIGNAL(editingFinished()));
  this->TextChangedAndEditingFinishedSpy =
    new QSignalSpy(this->TextEdit, SIGNAL(textChangedAndEditingFinished()));
}

// ----------------------------------------------------------------------------
void pqTextEditTester::cleanup()
{
  delete this->TextEdit;
  delete this->TextChangedSpy;
  delete this->EditingFinishedSpy;
  delete this->TextChangedAndEditingFinishedSpy;
}

// ----------------------------------------------------------------------------
QSignalSpy* pqTextEditTester::spy(int spyType)
{
  switch (spyType)
  {
    case pqTextEditTester::TextChanged:
    {
      return this->TextChangedSpy;
      break;
    }
    case pqTextEditTester::EditingFinished:
    {
      return this->EditingFinishedSpy;
      break;
    }
    case pqTextEditTester::TextChangedAndEditingFinished:
    {
      return this->TextChangedAndEditingFinishedSpy;
      break;
    }
    default:
    {
      qCritical("Developer Error. No such spy (that's what a spy would say)");
      return 0;
      break;
    }
  }
}

// ----------------------------------------------------------------------------
void pqTextEditTester::testSets()
{
  QFETCH(int, spyType);
  QFETCH(int, numberOfSignals);
  QFETCH(QString, text);

  this->TextEdit->setText(text);
  QCOMPARE(this->TextEdit->toPlainText(), text);
  QCOMPARE(this->spy(spyType)->count(), numberOfSignals);

  this->spy(spyType)->clear();
  this->TextEdit->setText(text);
  QCOMPARE(this->TextEdit->toPlainText(), text);
  QCOMPARE(this->spy(spyType)->count(), numberOfSignals);

  this->spy(spyType)->clear();
  this->TextEdit->setText("");
  QCOMPARE(this->TextEdit->toPlainText(), QString(""));
  QCOMPARE(this->spy(spyType)->count(), numberOfSignals);
}

// ----------------------------------------------------------------------------
void pqTextEditTester::testSets_data()
{
  QTest::addColumn<int>("spyType");
  QTest::addColumn<int>("numberOfSignals");
  QTest::addColumn<QString>("text");

  QTest::newRow("textChanged()") << 0 << 1 << "New Text";
  QTest::newRow("editingFinished()") << 1 << 0 << "New Text";
  QTest::newRow("textChangedAndEditingFinished()") << 2 << 0 << "New Text";
}

// ----------------------------------------------------------------------------
void pqTextEditTester::testTypeText()
{
  QFETCH(int, spyType);
  QFETCH(int, numberOfChangedSignals);
  QFETCH(QString, text);
  QFETCH(QString, expectedText);
  QFETCH(int, focusOutKey);
  QFETCH(int, focusOutModifier);
  QFETCH(bool, focus);

  this->TextEdit->show();
#if QT_VERSION >= 0x050000
  bool active = QTest::qWaitForWindowActive(this->TextEdit);
  if (!active)
  {
    qCritical() << "Window did not become active. Skipping testTypeText.";
    return;
  }
#else
  QTest::qWaitForWindowShown(this->TextEdit);
#endif
  this->TextEdit->setFocus();

  QTest::keyClicks(this->TextEdit, text);
  QTest::keyClick(this->TextEdit, static_cast<Qt::Key>(focusOutKey),
    static_cast<Qt::KeyboardModifier>(focusOutModifier));

  QCOMPARE(this->TextEdit->toPlainText(), expectedText);
  QCOMPARE(this->spy(spyType)->count(), numberOfChangedSignals);
  QCOMPARE(this->TextEdit->hasFocus(), focus);
}

// ----------------------------------------------------------------------------
void pqTextEditTester::testTypeText_data()
{
  QTest::addColumn<int>("spyType");
  QTest::addColumn<int>("numberOfChangedSignals");
  QTest::addColumn<QString>("text");
  QTest::addColumn<QString>("expectedText");
  QTest::addColumn<int>("focusOutKey");
  QTest::addColumn<int>("focusOutModifier");
  QTest::addColumn<bool>("focus");

  QString text = "My WEIrD CaSIng !@#$%)%^_*)[]{}|:'\" text.";

  QTest::newRow("textChanged: Key_A no modifier")
    << 0 << text.length() + 1 << text << text + QTest::keyToAscii(Qt::Key_A)
    << static_cast<int>(Qt::Key_A) << static_cast<int>(Qt::NoModifier) << true;
  QTest::newRow("textChanged: Key_Return and SHIFT")
    << 0 << text.length() + 1 << text << text + "\n"
    << static_cast<int>(Qt::Key_Return) << static_cast<int>(Qt::ShiftModifier) << true;

  QTest::newRow("textChanged: Key_Return and CTRL")
    << 0 << text.length() << text << text << static_cast<int>(Qt::Key_Return)
    << static_cast<int>(Qt::ControlModifier) << false;
  QTest::newRow("textChanged: Key_Return and ALT") << 0 << text.length() << text << text
                                                   << static_cast<int>(Qt::Key_Return)
                                                   << static_cast<int>(Qt::AltModifier) << false;
  QTest::newRow("textChanged: Key_Enter and CTRL")
    << 0 << text.length() << text << text << static_cast<int>(Qt::Key_Enter)
    << static_cast<int>(Qt::ControlModifier) << false;
  QTest::newRow("textChanged: Key_Enter and ALT") << 0 << text.length() << text << text
                                                  << static_cast<int>(Qt::Key_Enter)
                                                  << static_cast<int>(Qt::AltModifier) << false;

  QTest::newRow("editingFinished: Key_A no modifier")
    << 1 << 0 << text << text + QTest::keyToAscii(Qt::Key_A) << static_cast<int>(Qt::Key_A)
    << static_cast<int>(Qt::NoModifier) << true;
  QTest::newRow("editingFinished: Key_Return and SHIFT")
    << 1 << 0 << text << text + "\n"
    << static_cast<int>(Qt::Key_Return) << static_cast<int>(Qt::ShiftModifier) << true;

  QTest::newRow("editingFinished: Key_Return and CTRL")
    << 1 << 1 << text << text << static_cast<int>(Qt::Key_Return)
    << static_cast<int>(Qt::ControlModifier) << false;
  QTest::newRow("editingFinished: Key_Return and ALT")
    << 1 << 1 << text << text << static_cast<int>(Qt::Key_Return)
    << static_cast<int>(Qt::AltModifier) << false;
  QTest::newRow("editingFinished: Key_Enter and CTRL")
    << 1 << 1 << text << text << static_cast<int>(Qt::Key_Enter)
    << static_cast<int>(Qt::ControlModifier) << false;
  QTest::newRow("editingFinished: Key_Enter and ALT") << 1 << 1 << text << text
                                                      << static_cast<int>(Qt::Key_Enter)
                                                      << static_cast<int>(Qt::AltModifier) << false;

  QTest::newRow("textChangedAndEditingFinished: Key_A no modifier")
    << 2 << 0 << text << text + QTest::keyToAscii(Qt::Key_A) << static_cast<int>(Qt::Key_A)
    << static_cast<int>(Qt::NoModifier) << true;
  QTest::newRow("textChangedAndEditingFinished: Key_Return and SHIFT")
    << 2 << 0 << text << text + "\n"
    << static_cast<int>(Qt::Key_Return) << static_cast<int>(Qt::ShiftModifier) << true;

  QTest::newRow("textChangedAndEditingFinished: Key_Return and CTRL")
    << 2 << 1 << text << text << static_cast<int>(Qt::Key_Return)
    << static_cast<int>(Qt::ControlModifier) << false;
  QTest::newRow("textChangedAndEditingFinished: Key_Return and ALT")
    << 2 << 1 << text << text << static_cast<int>(Qt::Key_Return)
    << static_cast<int>(Qt::AltModifier) << false;
  QTest::newRow("textChangedAndEditingFinished: Key_Enter and CTRL")
    << 2 << 1 << text << text << static_cast<int>(Qt::Key_Enter)
    << static_cast<int>(Qt::ControlModifier) << false;
  QTest::newRow("textChangedAndEditingFinished: Key_Enter and ALT")
    << 2 << 1 << text << text << static_cast<int>(Qt::Key_Enter)
    << static_cast<int>(Qt::AltModifier) << false;
}

// ----------------------------------------------------------------------------
void pqTextEditTester::testFocus()
{
  QFETCH(int, focusOutKey);
  QFETCH(int, focusOutModifier);
  QFETCH(bool, focus);

  this->TextEdit->show();
#if QT_VERSION >= 0x050000
  bool active = QTest::qWaitForWindowActive(this->TextEdit);
  if (!active)
  {
    qCritical() << "Window did not become active. Skipping testFocus.";
    return;
  }
#else
  QTest::qWaitForWindowShown(this->TextEdit);
#endif
  this->TextEdit->setFocus();

  QTest::keyClick(this->TextEdit, static_cast<Qt::Key>(focusOutKey),
    static_cast<Qt::KeyboardModifier>(focusOutModifier));
  QCOMPARE(this->TextEdit->hasFocus(), focus);

  this->TextEdit->setFocus();
  this->TextEdit->clearFocus();
  QCOMPARE(this->TextEdit->hasFocus(), false);
}

// ----------------------------------------------------------------------------
void pqTextEditTester::testFocus_data()
{

  QTest::addColumn<int>("focusOutKey");
  QTest::addColumn<int>("focusOutModifier");
  QTest::addColumn<bool>("focus");

  QTest::newRow("textChanged: Key_A no modifier") << static_cast<int>(Qt::Key_A)
                                                  << static_cast<int>(Qt::NoModifier) << true;
  QTest::newRow("textChanged: Key_Return and SHIFT") << static_cast<int>(Qt::Key_Return)
                                                     << static_cast<int>(Qt::ShiftModifier) << true;
  QTest::newRow("textChanged: Key_Enter and SHIFT") << static_cast<int>(Qt::Key_Enter)
                                                    << static_cast<int>(Qt::ShiftModifier) << true;

  QTest::newRow("textChanged: Key_Return and CTRL")
    << static_cast<int>(Qt::Key_Return) << static_cast<int>(Qt::ControlModifier) << false;
  QTest::newRow("textChanged: Key_Return and ALT") << static_cast<int>(Qt::Key_Return)
                                                   << static_cast<int>(Qt::AltModifier) << false;
  QTest::newRow("textChanged: Key_Enter and CTRL")
    << static_cast<int>(Qt::Key_Enter) << static_cast<int>(Qt::ControlModifier) << false;
  QTest::newRow("textChanged: Key_Enter and ALT") << static_cast<int>(Qt::Key_Enter)
                                                  << static_cast<int>(Qt::AltModifier) << false;

  QTest::newRow("editingFinished: Key_A no modifier") << static_cast<int>(Qt::Key_A)
                                                      << static_cast<int>(Qt::NoModifier) << true;
  QTest::newRow("editingFinished: Key_Return and SHIFT")
    << static_cast<int>(Qt::Key_Return) << static_cast<int>(Qt::ShiftModifier) << true;
  QTest::newRow("editingFinished: Key_Enter and SHIFT")
    << static_cast<int>(Qt::Key_Enter) << static_cast<int>(Qt::ShiftModifier) << true;

  QTest::newRow("editingFinished: Key_Return and CTRL")
    << static_cast<int>(Qt::Key_Return) << static_cast<int>(Qt::ControlModifier) << false;
  QTest::newRow("editingFinished: Key_Return and ALT")
    << static_cast<int>(Qt::Key_Return) << static_cast<int>(Qt::AltModifier) << false;
  QTest::newRow("editingFinished: Key_Enter and CTRL")
    << static_cast<int>(Qt::Key_Enter) << static_cast<int>(Qt::ControlModifier) << false;
  QTest::newRow("editingFinished: Key_Enter and ALT") << static_cast<int>(Qt::Key_Enter)
                                                      << static_cast<int>(Qt::AltModifier) << false;

  QTest::newRow("textChangedAndEditingFinished: Key_A no modifier")
    << static_cast<int>(Qt::Key_A) << static_cast<int>(Qt::NoModifier) << true;
  QTest::newRow("textChangedAndEditingFinished: Key_Return and SHIFT")
    << static_cast<int>(Qt::Key_Return) << static_cast<int>(Qt::ShiftModifier) << true;
  QTest::newRow("textChangedAndEditingFinished: Key_Enter and SHIFT")
    << static_cast<int>(Qt::Key_Enter) << static_cast<int>(Qt::ShiftModifier) << true;

  QTest::newRow("textChangedAndEditingFinished: Key_Return and CTRL")
    << static_cast<int>(Qt::Key_Return) << static_cast<int>(Qt::ControlModifier) << false;
  QTest::newRow("textChangedAndEditingFinished: Key_Return and ALT")
    << static_cast<int>(Qt::Key_Return) << static_cast<int>(Qt::AltModifier) << false;
  QTest::newRow("textChangedAndEditingFinished: Key_Enter and CTRL")
    << static_cast<int>(Qt::Key_Enter) << static_cast<int>(Qt::ControlModifier) << false;
  QTest::newRow("textChangedAndEditingFinished: Key_Enter and ALT")
    << static_cast<int>(Qt::Key_Enter) << static_cast<int>(Qt::AltModifier) << false;
}

// ----------------------------------------------------------------------------
void pqTextEditTester::testReTypeText()
{
  QFETCH(int, spyType);
  QFETCH(int, numberOfChangedSignals);
  QFETCH(QString, text);
  QFETCH(QString, expectedText);
  QFETCH(int, focusOutKey);
  QFETCH(int, focusOutModifier);
  QFETCH(bool, focus);

  this->TextEdit->append(text); // Init Text
  this->spy(spyType)->clear();  // Reset spy

  this->TextEdit->show();
#if QT_VERSION >= 0x050000
  bool active = QTest::qWaitForWindowActive(this->TextEdit);
  if (!active)
  {
    qCritical() << "Window did not become active. Skipping testReTypeText.";
    return;
  }
#else
  QTest::qWaitForWindowShown(this->TextEdit);
#endif
  this->TextEdit->setFocus();

  for (int i = 0; i < text.length(); ++i) // Remove previous text
  {
    QTest::keyClick(this->TextEdit, Qt::Key_Backspace);
  }

  QTest::keyClicks(this->TextEdit, text); // Re-type it.
  QTest::keyClick(this->TextEdit, static_cast<Qt::Key>(focusOutKey),
    static_cast<Qt::KeyboardModifier>(focusOutModifier));

  QCOMPARE(this->TextEdit->toPlainText(), expectedText);
  QCOMPARE(this->spy(spyType)->count(), numberOfChangedSignals);
  QCOMPARE(this->TextEdit->hasFocus(), focus);
}

// ----------------------------------------------------------------------------
void pqTextEditTester::testReTypeText_data()
{
  QTest::addColumn<int>("spyType");
  QTest::addColumn<int>("numberOfChangedSignals");
  QTest::addColumn<QString>("text");
  QTest::addColumn<QString>("expectedText");
  QTest::addColumn<int>("focusOutKey");
  QTest::addColumn<int>("focusOutModifier");
  QTest::addColumn<bool>("focus");

  QString text = "My WEIrD CaSIng !@#$%)%^_*)[]{}|:'\" text.";

  // textChanged signal is sent 2 times as many text letters.
  // Once after each backspace with the new shortened text.
  // Once after each letter is retyped

  QTest::newRow("textChanged: Key_A no modifier")
    << 0 << 2 * text.length() + 1 << text << text + QTest::keyToAscii(Qt::Key_A)
    << static_cast<int>(Qt::Key_A) << static_cast<int>(Qt::NoModifier) << true;
  QTest::newRow("textChanged: Key_Return and SHIFT")
    << 0 << 2 * text.length() + 1 << text << text + "\n"
    << static_cast<int>(Qt::Key_Return) << static_cast<int>(Qt::ShiftModifier) << true;

  QTest::newRow("textChanged: Key_Return and CTRL")
    << 0 << 2 * text.length() << text << text << static_cast<int>(Qt::Key_Return)
    << static_cast<int>(Qt::ControlModifier) << false;
  QTest::newRow("textChanged: Key_Return and ALT") << 0 << 2 * text.length() << text << text
                                                   << static_cast<int>(Qt::Key_Return)
                                                   << static_cast<int>(Qt::AltModifier) << false;
  QTest::newRow("textChanged: Key_Enter and CTRL")
    << 0 << 2 * text.length() << text << text << static_cast<int>(Qt::Key_Enter)
    << static_cast<int>(Qt::ControlModifier) << false;
  QTest::newRow("textChanged: Key_Enter and ALT") << 0 << 2 * text.length() << text << text
                                                  << static_cast<int>(Qt::Key_Enter)
                                                  << static_cast<int>(Qt::AltModifier) << false;

  QTest::newRow("editingFinished: Key_A no modifier")
    << 1 << 0 << text << text + QTest::keyToAscii(Qt::Key_A) << static_cast<int>(Qt::Key_A)
    << static_cast<int>(Qt::NoModifier) << true;
  QTest::newRow("editingFinished: Key_Return and SHIFT")
    << 1 << 0 << text << text + "\n"
    << static_cast<int>(Qt::Key_Return) << static_cast<int>(Qt::ShiftModifier) << true;

  QTest::newRow("editingFinished: Key_Return and CTRL")
    << 1 << 1 << text << text << static_cast<int>(Qt::Key_Return)
    << static_cast<int>(Qt::ControlModifier) << false;
  QTest::newRow("editingFinished: Key_Return and ALT")
    << 1 << 1 << text << text << static_cast<int>(Qt::Key_Return)
    << static_cast<int>(Qt::AltModifier) << false;
  QTest::newRow("editingFinished: Key_Enter and CTRL")
    << 1 << 1 << text << text << static_cast<int>(Qt::Key_Enter)
    << static_cast<int>(Qt::ControlModifier) << false;
  QTest::newRow("editingFinished: Key_Enter and ALT") << 1 << 1 << text << text
                                                      << static_cast<int>(Qt::Key_Enter)
                                                      << static_cast<int>(Qt::AltModifier) << false;

  // Even though the text is the same, the textChangedAndEditingFinished
  // signal is still sent. See pqTextEdit doc.

  QTest::newRow("textChangedAndEditingFinished: Key_A no modifier")
    << 2 << 0 << text << text + QTest::keyToAscii(Qt::Key_A) << static_cast<int>(Qt::Key_A)
    << static_cast<int>(Qt::NoModifier) << true;
  QTest::newRow("textChangedAndEditingFinished: Key_Return and SHIFT")
    << 2 << 0 << text << text + "\n"
    << static_cast<int>(Qt::Key_Return) << static_cast<int>(Qt::ShiftModifier) << true;

  QTest::newRow("textChangedAndEditingFinished: Key_Return and CTRL")
    << 2 << 1 << text << text << static_cast<int>(Qt::Key_Return)
    << static_cast<int>(Qt::ControlModifier) << false;
  QTest::newRow("textChangedAndEditingFinished: Key_Return and ALT")
    << 2 << 1 << text << text << static_cast<int>(Qt::Key_Return)
    << static_cast<int>(Qt::AltModifier) << false;
  QTest::newRow("textChangedAndEditingFinished: Key_Enter and CTRL")
    << 2 << 1 << text << text << static_cast<int>(Qt::Key_Enter)
    << static_cast<int>(Qt::ControlModifier) << false;
  QTest::newRow("textChangedAndEditingFinished: Key_Enter and ALT")
    << 2 << 1 << text << text << static_cast<int>(Qt::Key_Enter)
    << static_cast<int>(Qt::AltModifier) << false;
}

// --------------------------------------------------------------------------
int pqTextEditTest(int argc, char* argv[])
{
  QApplication app(argc, argv);
  pqTextEditTester test1;
  return QTest::qExec(&test1, argc, argv);
}
