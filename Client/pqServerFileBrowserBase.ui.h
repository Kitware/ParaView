#ifndef PQSERVERFILEBROWSERBASE_H
#define PQSERVERFILEBROWSERBASE_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QDialog>
#include <QtGui/QGridLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QPushButton>
#include <QtGui/QSpacerItem>
#include "pqServerFileBrowserList.h"

class Ui_pqServerFileBrowserBase
{
public:
    QGridLayout *gridLayout;
    QHBoxLayout *hboxLayout;
    QPushButton *buttonUp;
    QPushButton *buttonHome;
    QSpacerItem *spacerItem;
    QPushButton *buttonOk;
    QPushButton *buttonCancel;
    pqServerFileBrowserList *fileList;

    void setupUi(QDialog *pqServerFileBrowserBase)
    {
    pqServerFileBrowserBase->setObjectName(QString::fromUtf8("pqServerFileBrowserBase"));
    pqServerFileBrowserBase->resize(QSize(511, 282).expandedTo(pqServerFileBrowserBase->minimumSizeHint()));
    pqServerFileBrowserBase->setSizeGripEnabled(true);
    gridLayout = new QGridLayout(pqServerFileBrowserBase);
    gridLayout->setSpacing(6);
    gridLayout->setMargin(10);
    gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
    hboxLayout = new QHBoxLayout();
    hboxLayout->setSpacing(6);
    hboxLayout->setMargin(0);
    hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
    buttonUp = new QPushButton(pqServerFileBrowserBase);
    buttonUp->setObjectName(QString::fromUtf8("buttonUp"));

    hboxLayout->addWidget(buttonUp);

    buttonHome = new QPushButton(pqServerFileBrowserBase);
    buttonHome->setObjectName(QString::fromUtf8("buttonHome"));

    hboxLayout->addWidget(buttonHome);

    spacerItem = new QSpacerItem(70, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    hboxLayout->addItem(spacerItem);

    buttonOk = new QPushButton(pqServerFileBrowserBase);
    buttonOk->setObjectName(QString::fromUtf8("buttonOk"));
    buttonOk->setAutoDefault(true);
    buttonOk->setDefault(true);

    hboxLayout->addWidget(buttonOk);

    buttonCancel = new QPushButton(pqServerFileBrowserBase);
    buttonCancel->setObjectName(QString::fromUtf8("buttonCancel"));
    buttonCancel->setAutoDefault(true);

    hboxLayout->addWidget(buttonCancel);


    gridLayout->addLayout(hboxLayout, 1, 0, 1, 1);

    fileList = new pqServerFileBrowserList(pqServerFileBrowserBase);
    fileList->setObjectName(QString::fromUtf8("fileList"));

    gridLayout->addWidget(fileList, 0, 0, 1, 1);

    retranslateUi(pqServerFileBrowserBase);
    QObject::connect(buttonOk, SIGNAL(clicked()), pqServerFileBrowserBase, SLOT(accept()));
    QObject::connect(buttonCancel, SIGNAL(clicked()), pqServerFileBrowserBase, SLOT(reject()));

    QMetaObject::connectSlotsByName(pqServerFileBrowserBase);
    } // setupUi

    void retranslateUi(QDialog *pqServerFileBrowserBase)
    {
    pqServerFileBrowserBase->setWindowTitle(QApplication::translate("pqServerFileBrowserBase", "Open File:", 0, QApplication::UnicodeUTF8));
    buttonUp->setText(QApplication::translate("pqServerFileBrowserBase", "Up Dir", 0, QApplication::UnicodeUTF8));
    buttonHome->setText(QApplication::translate("pqServerFileBrowserBase", "Home", 0, QApplication::UnicodeUTF8));
    buttonOk->setText(QApplication::translate("pqServerFileBrowserBase", "&OK", 0, QApplication::UnicodeUTF8));
    buttonOk->setShortcut(QApplication::translate("pqServerFileBrowserBase", "", 0, QApplication::UnicodeUTF8));
    buttonCancel->setText(QApplication::translate("pqServerFileBrowserBase", "&Cancel", 0, QApplication::UnicodeUTF8));
    buttonCancel->setShortcut(QApplication::translate("pqServerFileBrowserBase", "", 0, QApplication::UnicodeUTF8));
    Q_UNUSED(pqServerFileBrowserBase);
    } // retranslateUi

};

namespace Ui {
    class pqServerFileBrowserBase: public Ui_pqServerFileBrowserBase {};
} // namespace Ui

#endif // PQSERVERFILEBROWSERBASE_H
