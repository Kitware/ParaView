/*=========================================================================

   Program: ParaView
   Module:  pqSeriesGeneratorDialog.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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

========================================================================*/
#include "pqSeriesGeneratorDialog.h"
#include "ui_pqSeriesGeneratorDialog.h"

#include <QPushButton>
#include <QTextStream>

#include <cassert>
#include <cmath>

class pqSeriesGeneratorDialog::pqInternals
{
public:
  enum Mode
  {
    LINEAR,
    LOGARITHMIC,
    GEOMETRIC_SAMPLES,
    GEOMETRIC_RATIO,
  };
  Ui::SeriesGeneratorDialog Ui;

  bool validate();
  QVector<double> values() const;
};

//-----------------------------------------------------------------------------
bool pqSeriesGeneratorDialog::pqInternals::validate()
{
  auto& ui = this->Ui;
  const double start = ui.min->text().toDouble();
  const double end = ui.max->text().toDouble();
  const double min = std::min(start, end);
  const double max = std::max(start, end);

  QString msg;
  switch (ui.comboBox->currentIndex())
  {
    case LINEAR: // linear
      break;

    case LOGARITHMIC: // log
      if (min < 0.0 && max > 0.0)
      {
        msg = "Error: range cannot contain 0 for log.";
      }
      break;

    case GEOMETRIC_SAMPLES:
      if (start == 0.0 || end == 0.0)
      {
        msg = "Error: range cannot begin or end with 0 for a geometric series.";
      }
      else if (min < 0.0 && max > 0.0)
      {
        msg = "Error: range cannot contain 0 for a geometric series.";
      }
      break;

    case GEOMETRIC_RATIO:
      if (start == 0.0)
      {
        msg = "Error: range cannot begin with 0 for a geometric series.";
      }
      else if (ui.ratio->text().toDouble() == 0.0)
      {
        msg = "Error: common ratio cannot be 0 for a geometric series.";
      }
      break;
  }

  ui.message->setText(msg);
  auto generateButton = ui.buttonBox->button(QDialogButtonBox::Ok);
  generateButton->setEnabled(msg.isEmpty());

  if (msg.isEmpty())
  {
    auto sampleValues = this->values();
    QString txt;
    QTextStream stream(&txt);

    stream << "Sample series: ";
    bool add_comma = false;
    for (auto& val : sampleValues)
    {
      stream << (add_comma ? ", " : "") << val;
      add_comma = true;
    }
    ui.message->setText(txt);
    return true;
  }

  return false;
}

//-----------------------------------------------------------------------------
QVector<double> pqSeriesGeneratorDialog::pqInternals::values() const
{
  auto& ui = this->Ui;
  const int mode = ui.comboBox->currentIndex();
  const double start = ui.min->text().toDouble();
  const double end = ui.max->text().toDouble();
  const int nsamples = ui.nsamples->value();
  const double ratio = ui.ratio->text().toDouble();

  QVector<double> values;
  if (nsamples == 1)
  {
    values.push_back(start);
    return values;
  }
  else if (nsamples == 2)
  {
    values.push_back(start);
    values.push_back(end);
    return values;
  }

  assert(nsamples > 2);
  values.resize(nsamples);

  if (mode == LINEAR)
  {
    const double delta = (end - start) / (nsamples - 1);
    for (int cc = 0; cc < nsamples; ++cc)
    {
      values[cc] = start + delta * cc;
    }
  }
  else if (mode == LOGARITHMIC)
  {
    const double sign = start < 0 ? -1.0 : 1.0;
    const double log_start = std::log10(std::abs(start ? start : 1.0e-6 * (start - end)));
    const double log_end = std::log10(std::abs(end ? end : 1.0e-6 * (end - start)));

    const double delta = (log_end - log_start) / (nsamples - 1);
    for (int cc = 0; cc < nsamples; ++cc)
    {
      const double log_value = log_start + delta * cc;
      values[cc] = sign * pow(10.0, log_value);
    }
  }
  else if (mode == GEOMETRIC_SAMPLES)
  {
    const double log_cratio = std::log10(end / start) / (nsamples - 1);
    const double cratio = pow(10.0, log_cratio);

    for (int cc = 0; cc < nsamples; ++cc)
    {
      values[cc] = start * pow(cratio, cc);
    }
  }
  else if (mode == GEOMETRIC_RATIO)
  {
    assert(ratio != 0.0);
    for (int cc = 0; cc < nsamples; ++cc)
    {
      values[cc] = start * pow(ratio, cc);
    }
  }

  return values;
}

//-----------------------------------------------------------------------------
pqSeriesGeneratorDialog::pqSeriesGeneratorDialog(
  double min, double max, QWidget* parentObject, Qt::WindowFlags f)
  : Superclass(parentObject, f)
  , Internals(new pqSeriesGeneratorDialog::pqInternals())
{
  auto& internals = (*this->Internals);
  internals.Ui.setupUi(this);
  internals.Ui.min->setValidator(new QDoubleValidator(this));
  internals.Ui.max->setValidator(new QDoubleValidator(this));
  internals.Ui.ratio->setValidator(new QDoubleValidator(this));
  internals.Ui.label_ratio->hide();
  internals.Ui.ratio->hide();
  internals.Ui.buttonBox->button(QDialogButtonBox::Ok)->setText("&Generate");

  QObject::connect(internals.Ui.comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
    [&internals](int idx) {
      if (idx == pqInternals::GEOMETRIC_RATIO)
      {
        internals.Ui.max->setEnabled(false);
        internals.Ui.label_ratio->show();
        internals.Ui.ratio->show();
      }
      else
      {
        internals.Ui.max->setEnabled(true);
        internals.Ui.label_ratio->hide();
        internals.Ui.ratio->hide();
      }

      internals.validate();
    });

  auto callback = [&internals]() { internals.validate(); };
  QObject::connect(internals.Ui.max, &pqLineEdit::textChanged, callback);
  QObject::connect(internals.Ui.min, &pqLineEdit::textChanged, callback);
  QObject::connect(internals.Ui.ratio, &pqLineEdit::textChanged, callback);
  QObject::connect(internals.Ui.nsamples, QOverload<int>::of(&QSpinBox::valueChanged), callback);

  internals.Ui.min->setText(QVariant(min).toString());
  internals.Ui.max->setText(QVariant(max).toString());
}

//-----------------------------------------------------------------------------
pqSeriesGeneratorDialog::~pqSeriesGeneratorDialog()
{
}

//-----------------------------------------------------------------------------
QVector<double> pqSeriesGeneratorDialog::series() const
{
  auto& internals = (*this->Internals);
  return internals.validate() ? internals.values() : QVector<double>();
}
