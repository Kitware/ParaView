/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "pqConsoleWidget.h"

#include <QKeyEvent>
#include <QTextCursor>
#include <QTextEdit>

/////////////////////////////////////////////////////////////////////////
// pqConsoleWidget::pqImplementation

class pqConsoleWidget::pqImplementation :
  public QTextEdit
{
public:
  pqImplementation(pqConsoleWidget& parent) :
    QTextEdit(&parent),
    Parent(parent),
    InteractivePosition(documentEnd())
  {
    this->setTabChangesFocus(false);
    this->setAcceptDrops(false);
    
    QFont font;
    font.setStyleHint(QFont::TypeWriter);
    
    QTextCharFormat format;
    format.setFont(font);
    format.setForeground(QColor(0, 0, 0));
    this->setCurrentCharFormat(format);
    
    this->CommandHistory.append("");
    this->CommandPosition = 0;
  }

  void printString(const QString& Text)
  {
    this->textCursor().movePosition(QTextCursor::End);
    this->textCursor().insertText(Text);
    this->InteractivePosition = this->documentEnd();
    this->ensureCursorVisible();
  }

  void onCursorPositionChanged()
  {
    QTextCursor cursor = this->textCursor();
    if(cursor.position() < this->documentEnd())
      {
      cursor.setPosition(this->documentEnd());
      this->setTextCursor(cursor);
      }
  }
  
  void onSelectionChanged()
  {
    QTextCursor cursor = this->textCursor();
    if(cursor.position() < this->documentEnd())
      {
      cursor.setPosition(this->documentEnd());
      this->setTextCursor(cursor);
      }
  }

private:
  void keyPressEvent(QKeyEvent* Event)
  {
    switch(Event->key())
      {
      case Qt::Key_Left:
      case Qt::Key_Right:
        Event->accept();
        break;
      case Qt::Key_Up:
        Event->accept();
        if(this->CommandPosition > 0)
          this->replaceCommandBuffer(this->CommandHistory[--this->CommandPosition]);
        break;
      case Qt::Key_Down:
        Event->accept();
        if(this->CommandPosition < this->CommandHistory.size() - 2)
          this->replaceCommandBuffer(this->CommandHistory[++this->CommandPosition]);
        break;
      case Qt::Key_Backspace:
        Event->accept();
        if(this->commandBuffer().size())
          {
          this->commandBuffer().chop(1);
          QTextEdit::keyPressEvent(Event);
          }
        break;
      case Qt::Key_Return:
      case Qt::Key_Enter:
        Event->accept();
        this->internalExecuteCommand();
        break;
        break;
      default:
        this->commandBuffer().append(Event->text());
        QTextEdit::keyPressEvent(Event);
        break;
      }
  }
  
  /// Return the end of the document
  const int documentEnd()
  {
    QTextCursor cursor(this->document());
    cursor.movePosition(QTextCursor::End);
    return cursor.position();
  }
  
  /// Replace the contents of the command buffer, updating the display
  void replaceCommandBuffer(const QString& Text)
  {
    this->commandBuffer() = Text;
  
    QTextCursor cursor(this->document());
    cursor.setPosition(this->InteractivePosition);
    cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    cursor.removeSelectedText();
    cursor.insertText(Text);
  }
  
  /// References the buffer where the current un-executed command is stored
  QString& commandBuffer()
  {
    return this->CommandHistory.back();
  }
  
  /// Implements command-execution
  void internalExecuteCommand()
  {
    QTextCursor cursor(this->document());
    cursor.movePosition(QTextCursor::End);
    cursor.insertText("\n");

    this->Parent.internalExecuteCommand(this->commandBuffer());
    
    this->InteractivePosition = this->documentEnd();
    
    if(!this->commandBuffer().isEmpty()) // Don't store empty commands in the history
      {
      this->CommandHistory.append("");
      this->CommandPosition = this->CommandHistory.size() - 1;
      }
  }
  
  /// Stores a back-reference to our owner
  pqConsoleWidget& Parent;
  /// Stores the beginning of the area of interactive input, outside which the cursor can't be moved
  int InteractivePosition;
  /// Stores command-history, plus the current command buffer
  QStringList CommandHistory;
  /// Stores the current position in the command-history
  int CommandPosition;
};

/////////////////////////////////////////////////////////////////////////
// pqConsoleWidget

pqConsoleWidget::pqConsoleWidget(QWidget* Parent) :
  QFrame(Parent),
  Implementation(new pqImplementation(*this))
{
  this->setFrameShape(QFrame::NoFrame);

  QObject::connect(this->Implementation, SIGNAL(cursorPositionChanged()), this, SLOT(onCursorPositionChanged()));
  QObject::connect(this->Implementation, SIGNAL(selectionChanged()), this, SLOT(onSelectionChanged()));
}

pqConsoleWidget::~pqConsoleWidget()
{
  delete this->Implementation;
}

QTextCharFormat pqConsoleWidget::getFormat()
{
  return this->Implementation->currentCharFormat();
}

void pqConsoleWidget::setFormat(const QTextCharFormat& Format)
{
  this->Implementation->setCurrentCharFormat(Format);
}

void pqConsoleWidget::printString(const QString& Text)
{
  this->Implementation->printString(Text);
}

void pqConsoleWidget::internalExecuteCommand(const QString& Command)
{
  emit executeCommand(Command);
}

void pqConsoleWidget::resizeEvent(QResizeEvent* Event)
{
  this->Implementation->resize(Event->size());
}

void pqConsoleWidget::onCursorPositionChanged()
{
  this->Implementation->onCursorPositionChanged();
}

void pqConsoleWidget::onSelectionChanged()
{
  this->Implementation->onSelectionChanged();
}

