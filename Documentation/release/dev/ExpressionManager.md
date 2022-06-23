## Introduce the Expression Manager

Filters like [Python]Calculator expose a String Property to setup an expression to apply.
Any one liner can freely be typed in, but it used to be a manual operation, to be repeated
at each filter creation.

ParaView now provides an Expression Manager to ease this property configuration by storing expressions,
and giving quick access to them.
Each expression can be named, to help future reuse, and has an associated group
so it is easy to filter python expression from others.

It comes in two parts.
From the `Property Panel`, the one liner String Property Widget is extended with:
- a drop down list to access existing expressions
- a "save current expression" button
- a shortcut to the Expression Manager dialog
![Integration in Properties panel](../img/dev/ExpressionManager-PropertyIntegration.png "Expression Manager integration")

The Expression Manager Dialog, also available from the `Tools` menu, is an editable and searchable
list of the stored expressions.
ParaView keep track of them through the settings, but they can also be exported
to a json file for backup or share purpose.
![Expression Manager Dialog](../img/dev/ExpressionManager-Dialog.png "Expression Manager Dialog")
