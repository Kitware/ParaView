// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqWidgetUtilities.h"

#include "vtksys/RegularExpression.hxx"

#include <QRegularExpression>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextDocumentFragment>
#include <QTextTable>
#include <QToolTip>

#include <algorithm>
#include <cmath>

namespace
{
// imperfect way to generate rich-text with wrapping such that the rendered
// tooltip isn't too narrow, since Qt doesn't let us set the width explicitly
const int wrapLength = 70;
const int wrapWidth = 500;
}

//-----------------------------------------------------------------------------
QString pqWidgetUtilities::formatTooltip(const QString& rawText)
{
  if (rawText.isEmpty())
  {
    return QString();
  }

  QString doc = rstToHtml(rawText);
  doc = doc.trimmed();
  doc = doc.replace(QRegularExpression("\\s+"), " ");
  if (doc.size() < wrapLength)
  {
    // entire text is formatted without wrapping
    return QString("<html><head/><body><p style='white-space: pre'>%1</p></body></html>").arg(doc);
  }
  else
  {
    // text is wrapped by enforced table width
    return QString(
      "<html><head/><body><table width='%2'><tr><td>%1</td></tr></table></body></html>")
      .arg(doc)
      .arg(wrapWidth);
  }
}

//-----------------------------------------------------------------------------
// Markdown -> QTextDocument -> HTML
// the markdown text is placed in a table with fixed width inside QTextDocument
QString pqWidgetUtilities::formatMarkdownTooltip(const QString& markdown)
{
  const QString md = markdown.trimmed();
  if (md.isEmpty())
  {
    return QString();
  }

  // parse the Markdown into a scratch document so it can be measured and
  // inserted anywhere
  QTextDocument source;
  source.setDefaultFont(QToolTip::font());
  source.setMarkdown(md);
  const QTextDocumentFragment content(&source);

  // The text is placed in a table whose width is the unwrapped text width,
  // capped at wrapWidth. Short text stays on one line and long text is
  // wrapped at wrapWidth. (Non-breakable blocks are exported as <pre>, which
  // the tooltip ignores, so they cannot be used to prevent wrapping.)
  QTextDocument doc;
  // toHtml() writes the default font into the body style
  doc.setDefaultFont(QToolTip::font());
  QTextCursor cursor(&doc);
  QTextTableFormat tableFormat;
  tableFormat.setWidth(QTextLength(
    QTextLength::FixedLength, std::ceil(std::min(source.idealWidth(), qreal(wrapWidth)))));
  tableFormat.setBorder(0);
  tableFormat.setCellPadding(0);
  tableFormat.setCellSpacing(0);
  QTextTable* table = cursor.insertTable(1, 1, tableFormat);
  table->cellAt(0, 0).firstCursorPosition().insertFragment(content);
  return doc.toHtml();
}

//-----------------------------------------------------------------------------
QString pqWidgetUtilities::rstToHtml(const QString& rstStr)
{
  return QString::fromStdString(rstToHtml(rstStr.toUtf8().data()));
}

//-----------------------------------------------------------------------------
std::string pqWidgetUtilities::rstToHtml(const char* rstStr)
{
  std::string htmlStr = rstStr;
  {
    // bold
    vtksys::RegularExpression re("[*][*]([^*]+)[*][*]");
    while (re.find(htmlStr))
    {
      const char* s = htmlStr.c_str();
      std::string bold(s + re.start(1), re.end(1) - re.start(1));
      htmlStr.replace(re.start(0), re.end(0) - re.start(0), std::string("<b>") + bold + "</b>");
    }
  }
  {
    // italic
    vtksys::RegularExpression re("[*]([^*]+)[*]");
    while (re.find(htmlStr))
    {
      const char* s = htmlStr.c_str();
      std::string it(s + re.start(1), re.end(1) - re.start(1));
      htmlStr.replace(re.start(0), re.end(0) - re.start(0), std::string("<i>") + it + "</i>");
    }
  }
  {
    // begin bullet list
    size_t start = 0;
    std::string src("\n\n- ");
    while ((start = htmlStr.find(src, start)) != std::string::npos)
    {
      htmlStr.replace(start, src.size(), "\n<ul><li>");
    }
  }
  {
    // li for bullet list
    size_t start = 0;
    std::string src("\n- ");
    while ((start = htmlStr.find(src, start)) != std::string::npos)
    {
      htmlStr.replace(start, src.size(), "\n<li>");
    }
  }
  {
    // end bullet list
    vtksys::RegularExpression re("<li>(.*)\n\n([^-])");
    while (re.find(htmlStr))
    {
      const char* s = htmlStr.c_str();
      std::string listItem(s + re.start(1), re.end(1) - re.start(1));
      std::string afterList(s + re.start(2), re.end(2) - re.start(2));
      htmlStr.replace(re.start(0), re.end(0) - re.start(0),
        std::string("<li>").append(listItem).append("</ul>").append(afterList));
    }
  }
  {
    // paragraph
    size_t start = 0;
    std::string src("\n\n");
    while ((start = htmlStr.find(src, start)) != std::string::npos)
    {
      htmlStr.replace(start, src.size(), "\n<p>\n");
    }
  }
  return htmlStr;
}

//-----------------------------------------------------------------------------
void pqWidgetUtilities::formatChildTooltips(QWidget* parentObject)
{
  if (!parentObject)
  {
    return;
  }
  const QList<QWidget*> children = parentObject->findChildren<QWidget*>();
  for (QWidget* child : children)
  {
    QString tooltip = child->toolTip();
    if (!tooltip.isEmpty())
    {
      child->setToolTip(formatTooltip(tooltip));
    }
  }
}
