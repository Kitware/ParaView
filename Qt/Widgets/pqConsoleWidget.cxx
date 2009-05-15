/*=========================================================================

   Program: ParaView
   Module:    pqConsoleWidget.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2. 

   See License_v1.2.txt for the full ParaView license.
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

#include <QApplication>
#include <QClipboard>
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
    this->setAcceptRichText(false);
    this->setUndoRedoEnabled(false);
    
    QFont f;
    f.setFamily("Courier");
    f.setStyleHint(QFont::TypeWriter);
    f.setFixedPitch(true);
    
    QTextCharFormat format;
    format.setFont(f);
    format.setForeground(QColor(0, 0, 0));
    this->setCurrentCharFormat(format);
    
    this->CommandHistory.append("");
    this->CommandPosition = 0;
  }

  void keyPressEvent(QKeyEvent* e)
  {
    QTextCursor text_cursor = this->textCursor();

    // Set to true if there's a current selection
    const bool selection = text_cursor.anchor() != text_cursor.position();
    // Set to true if the cursor overlaps the history area
    const bool history_area =
      text_cursor.anchor() < this->InteractivePosition
      || text_cursor.position() < this->InteractivePosition;

    // Allow copying anywhere in the console ...
    if(e->key() == Qt::Key_C && e->modifiers() == Qt::ControlModifier)
      {
      if(selection)
        {
        this->copy();
        }
        
      e->accept();
      return;
      }
      
    // Allow cut only if the selection is limited to the interactive area ...
    if(e->key() == Qt::Key_X && e->modifiers() == Qt::ControlModifier)
      {
      if(selection && !history_area)
        {
        this->cut();
        }
        
      e->accept();
      return;
      }
    
    // Allow paste only if the selection is in the interactive area ...
    if(e->key() == Qt::Key_V && e->modifiers() == Qt::ControlModifier)
      {
      if(!history_area)
        {
        const QMimeData* const clipboard = QApplication::clipboard()->mimeData();
        const QString text = clipboard->text();
        if(!text.isNull())
          {
          text_cursor.insertText(text);
          this->updateCommandBuffer();
          }
        }
        
      e->accept();
      return;
      }
    
    // Force the cursor back to the interactive area
    if(history_area && e->key() != Qt::Key_Control)
      {
      text_cursor.setPosition(this->documentEnd());
      this->setTextCursor(text_cursor);
      }
    
    switch(e->key())
      {
      case Qt::Key_Up:
        e->accept();
        if (this->CommandPosition > 0)
          {
          this->replaceCommandBuffer(this->CommandHistory[--this->CommandPosition]);
          }
        break;
        
      case Qt::Key_Down:
        e->accept();
        if (this->CommandPosition < this->CommandHistory.size() - 2)
          {
          this->replaceCommandBuffer(this->CommandHistory[++this->CommandPosition]);
          }
        else
          {
          this->CommandPosition = this->CommandHistory.size()-1;
          this->replaceCommandBuffer("");
          }
        break;
  
      case Qt::Key_Delete:
        e->accept();
        QTextEdit::keyPressEvent(e);
        this->updateCommandBuffer();
        break;
        
      case Qt::Key_Backspace:
        e->accept();
        if(text_cursor.position() > this->InteractivePosition)
          {
          QTextEdit::keyPressEvent(e);
          this->updateCommandBuffer();
          }
        break;

      case Qt::Key_Home:
        e->accept();
        text_cursor.setPosition(this->InteractivePosition);
        this->setTextCursor(text_cursor);
        break;
        
      case Qt::Key_Return:
      case Qt::Key_Enter:
        e->accept();
        
        text_cursor.setPosition(this->documentEnd());
        this->setTextCursor(text_cursor);
        
        this->internalExecuteCommand();
        break;
        
      default:
        e->accept();
        QTextEdit::keyPressEvent(e);
        this->updateCommandBuffer();
        break;
      }
  }
  
  /// Returns the end of the document
  const int documentEnd()
  {
    QTextCursor c(this->document());
    c.movePosition(QTextCursor::End);
    return c.position();
  }
  
  /// Update the contents of the command buffer from the contents of the widget
  void updateCommandBuffer()
  {
    this->commandBuffer() = this->toPlainText().mid(this->InteractivePosition);
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
    // First update the history cache. It's essential to update the
    // this->CommandPosition before calling internalExecuteCommand() since that
    // can result in a clearing of the current command (BUG #8765).
    QString command = this->commandBuffer();
    if (!command.isEmpty()) // Don't store empty commands in the history
      {
      this->CommandHistory.push_back("");
      this->CommandPosition = this->CommandHistory.size() - 1;
      }
    QTextCursor c(this->document());
    c.movePosition(QTextCursor::End);
    c.insertText("\n");

    this->InteractivePosition = this->documentEnd();


    this->Parent.internalExecuteCommand(command);
    }
  
  /// Stores a back-reference to our owner
  pqConsoleWidget& Parent;
  /** Stores the beginning of the area of interactive input, outside which
  changes can't be made to the text edit contents */
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
}

//-----------------------------------------------------------------------------
pqConsoleWidget::~pqConsoleWidget()
{
  delete this->Implementation;
}

//-----------------------------------------------------------------------------
QTextCharFormat pqConsoleWidget::getFormat()
{
  return this->Implementation->currentCharFormat();
}

//-----------------------------------------------------------------------------
void pqConsoleWidget::setFormat(const QTextCharFormat& Format)
{
  this->Implementation->setCurrentCharFormat(Format);
}

//-----------------------------------------------------------------------------
void pqConsoleWidget::printString(const QString& Text)
{
  this->Implementation->textCursor().movePosition(QTextCursor::End);
  this->Implementation->textCursor().insertText(Text);
  this->Implementation->InteractivePosition = this->Implementation->documentEnd();
  this->Implementation->ensureCursorVisible();
}

//-----------------------------------------------------------------------------
void pqConsoleWidget::printCommand(const QString& cmd)
{
  this->Implementation->textCursor().insertText(cmd);
  this->Implementation->updateCommandBuffer();
}

//-----------------------------------------------------------------------------
void pqConsoleWidget::prompt(const QString& text)
{
  QTextCursor text_cursor = this->Implementation->textCursor();

  // if the cursor is currently on a clean line, do nothing, otherwise we move
  // the cursor to a new line before showing the prompt.
  text_cursor.movePosition(QTextCursor::StartOfLine);
  int startpos = text_cursor.position();
  text_cursor.movePosition(QTextCursor::EndOfLine);
  int endpos = text_cursor.position();
  if (endpos != startpos)
    {
    this->Implementation->textCursor().insertText("\n");
    }

  this->Implementation->textCursor().insertText(text);
  this->Implementation->InteractivePosition = this->Implementation->documentEnd();
  this->Implementation->ensureCursorVisible();
}

//-----------------------------------------------------------------------------
void pqConsoleWidget::clear()
{
  this->Implementation->clear();
}

//-----------------------------------------------------------------------------
void pqConsoleWidget::internalExecuteCommand(const QString& Command)
{
  emit this->executeCommand(Command);
}

//-----------------------------------------------------------------------------
