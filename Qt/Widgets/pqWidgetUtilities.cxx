// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqWidgetUtilities.h"

#include "vtksys/RegularExpression.hxx"

#include <QRegularExpression>

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
  // imperfect way to generate rich-text with wrapping such that the rendered
  // tooltip isn't too narrow, since Qt doesn't let us set the width explicitly
  int wrapLength = 70;
  int wrapWidth = 500;
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
