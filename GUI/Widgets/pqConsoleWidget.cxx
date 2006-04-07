/*=========================================================================

   Program:   ParaQ
   Module:    pqConsoleWidget.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "pqConsoleWidget.h"

#include <QKeyEvent>
#include <QTextCursor>
#include <QTextEdit>
#include <QVBoxLayout>

/////////////////////////////////////////////////////////////////////////
// pqConsoleWidget::pqImplementation

class pqConsoleWidget::pqImplementation :
  public QTextEdit
{
public:
  pqImplementation(pqConsoleWidget& p) :
    QTextEdit(&p),
    Parent(p),
    InteractivePosition(documentEnd())
  {
    this->setTabChangesFocus(false);
    this->setAcceptDrops(false);
    
    QFont f;
    f.setStyleHint(QFont::TypeWriter);
    
    QTextCharFormat format;
    format.setFont(f);
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
    QTextCursor c = this->textCursor();
    if(c.position() < this->documentEnd())
      {
      c.setPosition(this->documentEnd());
      this->setTextCursor(c);
      }
  }
  
  void onSelectionChanged()
  {
    QTextCursor c = this->textCursor();
    if(c.position() < this->documentEnd())
      {
      c.setPosition(this->documentEnd());
      this->setTextCursor(c);
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
    QTextCursor c(this->document());
    c.movePosition(QTextCursor::End);
    return c.position();
  }
  
  /// Replace the contents of the command buffer, updating the display
  void replaceCommandBuffer(const QString& Text)
  {
    this->commandBuffer() = Text;
  
    QTextCursor c(this->document());
    c.setPosition(this->InteractivePosition);
    c.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    c.removeSelectedText();
    c.insertText(Text);
  }
  
  /// References the buffer where the current un-executed command is stored
  QString& commandBuffer()
  {
    return this->CommandHistory.back();
  }
  
  /// Implements command-execution
  void internalExecuteCommand()
  {
    QTextCursor c(this->document());
    c.movePosition(QTextCursor::End);
    c.insertText("\n");

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
  QWidget(Parent),
  Implementation(new pqImplementation(*this))
{
  QVBoxLayout* const l = new QVBoxLayout(this);
  l->setMargin(0);
  l->addWidget(this->Implementation);

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

void pqConsoleWidget::onCursorPositionChanged()
{
  this->Implementation->onCursorPositionChanged();
}

void pqConsoleWidget::onSelectionChanged()
{
  this->Implementation->onSelectionChanged();
}

