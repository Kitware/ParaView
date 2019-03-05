/*=========================================================================

   Program: ParaView
   Module:  HierarchicalGridLayout.cxx

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
#include "HierarchicalGridLayout.h"
#include "pqHierarchicalGridLayout.h"
#include "pqHierarchicalGridWidget.h"

#include <QApplication>
#include <QFrame>
#include <QMessageBox>
#include <QTest>
#include <QWidget>

Q_DECLARE_METATYPE(pqHierarchicalGridLayout::Item);

void HierarchicalGridLayoutTester::addWidget_data()
{
  QTest::addColumn<int>("numberOfWidgets");
  QTest::addColumn<int>("maximize");

  QTest::newRow("1 widget") << 1 << -1; // deliberately passing invalid index.
  QTest::newRow("5 widgets") << 5 << 0;
  QTest::newRow("7 widgets") << 7 << 5;
  QTest::newRow("10 widgets") << 10 << 5;
}

void HierarchicalGridLayoutTester::addWidget()
{
  QWidget container;
  auto l = new pqHierarchicalGridLayout(&container);
  container.resize(400, 400);
  container.show();
  QTest::qWait(250);

  QVector<QWidget*> frames;
  QFETCH(int, numberOfWidgets);
  for (int cc = 0; cc < numberOfWidgets; ++cc)
  {
    auto f = new QFrame();
    f->setFrameShape(QFrame::Panel);
    f->setFrameShadow(QFrame::Raised);
    l->addWidget(f);
    frames.push_back(f);
    QTest::qWait(100);
  }

  QFETCH(int, maximize);
  l->maximize(maximize);
  QTest::qWait(100);
  l->maximize(0);
  QTest::qWait(100);

  // test remove widgets as well (don't remove all,
  // let's say remove up to 5).
  for (int cc = 0; cc < numberOfWidgets && cc < 5; ++cc)
  {
    l->removeWidget(frames[cc]);
    delete frames[cc];
    QTest::qWait(100);
  }
}

void HierarchicalGridLayoutTester::rearrange_data()
{
  using ItemT = pqHierarchicalGridLayout::Item;
  QTest::addColumn<QVector<QVector<ItemT> > >("sbtrees");
  QTest::addColumn<QVector<QSet<QWidget*> > >("removedWidgets");
  {
    // Case 1. [W0], [SH,null,W0], []
    QVector<QVector<ItemT> > sbtrees(3);
    QVector<QSet<QWidget*> > removedWidgets(3);

    // [W0]
    auto w0 = new QFrame();
    sbtrees[0].push_back(ItemT(w0));

    // [SH, null, W0]
    sbtrees[1].push_back(ItemT(Qt::Horizontal, 0.3));
    sbtrees[1].push_back(ItemT(nullptr));
    sbtrees[1].push_back(ItemT(w0));

    // []
    removedWidgets[2].insert(w0);

    QTest::newRow("case 1: '[W0], [SH,null,W0], []'") << sbtrees << removedWidgets;
  }

  {
    // Case 2. [], [W0], [SV, W0, W1], [SH, SV, W1, W0, null, W2]
    QVector<QVector<ItemT> > sbtrees(3);
    QVector<QSet<QWidget*> > removedWidgets(3);

    auto w0 = new QFrame();
    auto w1 = new QFrame();
    auto w2 = new QFrame();

    // []

    // [W0]
    sbtrees[0].push_back(ItemT(w0));

    // [SV, W0, W1]
    sbtrees[1].push_back(ItemT(Qt::Vertical, 0.5));
    sbtrees[1].push_back(ItemT(w0));
    sbtrees[1].push_back(ItemT(w1));

    // [SH, SV, W1, W0, null, W2]
    sbtrees[2].push_back(ItemT(Qt::Horizontal, 0.5));
    sbtrees[2].push_back(ItemT(Qt::Vertical, 0.5));
    sbtrees[2].push_back(ItemT(w1));
    sbtrees[2].push_back(ItemT(w0));
    sbtrees[2].push_back(ItemT(nullptr));
    sbtrees[2].push_back(ItemT(w2));
    removedWidgets[2].insert(w2);

    QTest::newRow("case 2: '[], [W0], [SV, W0, W1], [SH, SV, W1, W0, null, W2]'") << sbtrees
                                                                                  << removedWidgets;
  }
}

void HierarchicalGridLayoutTester::rearrange()
{
  pqHierarchicalGridWidget container;
  auto l = new pqHierarchicalGridLayout(&container);
  container.resize(400, 400);
  container.show();
  QTest::qWait(250);

  using ItemT = pqHierarchicalGridLayout::Item;
  QFETCH(QVector<QVector<ItemT> >, sbtrees);
  QFETCH(QVector<QSet<QWidget*> >, removedWidgets);

  QVERIFY(sbtrees.size() == removedWidgets.size());

  QSet<QWidget*> allremovedWidgets;
  for (int idx = 0; idx < sbtrees.size(); ++idx)
  {
    auto result = l->rearrange(sbtrees[idx]);

    QSet<QWidget*> resultSet;
    for (auto w : result)
    {
      resultSet.insert(w);
    }
    allremovedWidgets.unite(resultSet);

    QVERIFY(resultSet == removedWidgets[idx]);
  }

  for (auto w : allremovedWidgets)
  {
    delete w;
  }
}

void HierarchicalGridLayoutTester::interactiveResize()
{
  pqHierarchicalGridWidget container;
  auto l = new pqHierarchicalGridLayout(&container);
  l->setSpacing(4);

  auto f0 = new QFrame();
  f0->setFrameShape(QFrame::Panel);
  f0->setFrameShadow(QFrame::Raised);

  auto f1 = new QFrame();
  f1->setFrameShape(QFrame::Panel);
  f1->setFrameShadow(QFrame::Raised);

  l->addWidget(f0);
  l->addWidget(f1);
  container.resize(400, 400);
  container.show();
  QTest::qWait(250);

  QCOMPARE(f1->width(), 198);
  QCOMPARE(f0->width(), 198);

  // click at center, nothing should happen.
  QTest::mouseClick(&container, Qt::LeftButton);
  QCOMPARE(f1->width(), 198);
  QCOMPARE(f0->width(), 198);

  // press and move, widgets should resize.
  auto center = QPoint(200, 200);
  QTest::mousePress(&container, Qt::LeftButton, Qt::NoModifier, center, 200);
  QTest::mouseMove(&container, center + QPoint(1, 1), 100);
  QTest::mouseRelease(&container, Qt::LeftButton, Qt::NoModifier, center + QPoint(50, 50), 100);
  QTest::qWait(250);
}

int HierarchicalGridLayout(int argc, char* argv[])
{
  QApplication app(argc, argv);
  HierarchicalGridLayoutTester tester;
  return QTest::qExec(&tester, argc, argv);
}
