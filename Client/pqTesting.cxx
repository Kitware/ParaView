/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "pqTestCases.h"
#include "pqTesting.h"

#include <QtTest>

namespace
{

template<typename TestT>
void pqRunRegressionTest()
{
  TestT test;
  QtTest::exec(&test);
}

template<typename TestT>
void pqRunRegressionTest(QWidget& RootWidget)
{
  TestT test(RootWidget);
  QtTest::exec(&test);
}

} // namespace

void pqRunRegressionTests()
{
  pqRunRegressionTest<pqTestTestingFramework>();
}

void pqRunRegressionTests(QWidget& RootWidget)
{
  pqRunRegressionTest<pqTestFileMenu>(RootWidget);
}

