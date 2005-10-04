#ifndef PQSERVERFILEBROWSER_H
#define PQSERVERFILEBROWSER_H

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

class Ui_pqServerFileBrowser
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

    void setupUi(QDialog *pqServerFileBrowser)
    {
    pqServerFileBrowser->setObjectName(QString::fromUtf8("pqServerFileBrowser"));
    pqServerFileBrowser->resize(QSize(511, 282).expandedTo(pqServerFileBrowser->minimumSizeHint()));
    pqServerFileBrowser->setSizeGripEnabled(true);
    gridLayout = new QGridLayout(pqServerFileBrowser);
    gridLayout->setSpacing(6);
    gridLayout->setMargin(10);
    gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
    hboxLayout = new QHBoxLayout();
    hboxLayout->setSpacing(6);
    hboxLayout->setMargin(0);
    hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
    buttonUp = new QPushButton(pqServerFileBrowser);
    buttonUp->setObjectName(QString::fromUtf8("buttonUp"));

    hboxLayout->addWidget(buttonUp);

    buttonHome = new QPushButton(pqServerFileBrowser);
    buttonHome->setObjectName(QString::fromUtf8("buttonHome"));

    hboxLayout->addWidget(buttonHome);

    spacerItem = new QSpacerItem(70, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    hboxLayout->addItem(spacerItem);

    buttonOk = new QPushButton(pqServerFileBrowser);
    buttonOk->setObjectName(QString::fromUtf8("buttonOk"));
    buttonOk->setAutoDefault(true);
    buttonOk->setDefault(true);

    hboxLayout->addWidget(buttonOk);

    buttonCancel = new QPushButton(pqServerFileBrowser);
    buttonCancel->setObjectName(QString::fromUtf8("buttonCancel"));
    buttonCancel->setAutoDefault(true);

    hboxLayout->addWidget(buttonCancel);


    gridLayout->addLayout(hboxLayout, 1, 0, 1, 1);

    fileList = new pqServerFileBrowserList(pqServerFileBrowser);
    fileList->setObjectName(QString::fromUtf8("fileList"));

    gridLayout->addWidget(fileList, 0, 0, 1, 1);

    retranslateUi(pqServerFileBrowser);
    QObject::connect(buttonOk, SIGNAL(clicked()), pqServerFileBrowser, SLOT(accept()));
    QObject::connect(buttonCancel, SIGNAL(clicked()), pqServerFileBrowser, SLOT(reject()));

    QMetaObject::connectSlotsByName(pqServerFileBrowser);
    } // setupUi

    void retranslateUi(QDialog *pqServerFileBrowser)
    {
    pqServerFileBrowser->setWindowTitle(QApplication::translate("pqServerFileBrowser", "Open File:", 0, QApplication::UnicodeUTF8));
    buttonUp->setText(QApplication::translate("pqServerFileBrowser", "Up Dir", 0, QApplication::UnicodeUTF8));
    buttonHome->setText(QApplication::translate("pqServerFileBrowser", "Home", 0, QApplication::UnicodeUTF8));
    buttonOk->setText(QApplication::translate("pqServerFileBrowser", "&OK", 0, QApplication::UnicodeUTF8));
    buttonOk->setShortcut(QApplication::translate("pqServerFileBrowser", "", 0, QApplication::UnicodeUTF8));
    buttonCancel->setText(QApplication::translate("pqServerFileBrowser", "&Cancel", 0, QApplication::UnicodeUTF8));
    buttonCancel->setShortcut(QApplication::translate("pqServerFileBrowser", "", 0, QApplication::UnicodeUTF8));
    Q_UNUSED(pqServerFileBrowser);
    } // retranslateUi

};

namespace Ui {
    class pqServerFileBrowser: public Ui_pqServerFileBrowser {};
} // namespace Ui

#endif // PQSERVERFILEBROWSER_H
