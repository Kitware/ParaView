#ifndef PQABOUT_H
#define PQABOUT_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QDialog>
#include <QtGui/QGridLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QSpacerItem>

class Ui_pqAboutDialog
{
public:
    QGridLayout *gridLayout;
    QLabel *label;
    QHBoxLayout *hboxLayout;
    QSpacerItem *spacerItem;
    QPushButton *okButton;

    void setupUi(QDialog *pqAboutDialog)
    {
    pqAboutDialog->setObjectName(QString::fromUtf8("pqAboutDialog"));
    pqAboutDialog->resize(QSize(364, 227).expandedTo(pqAboutDialog->minimumSizeHint()));
    QSizePolicy sizePolicy((QSizePolicy::Policy)1, (QSizePolicy::Policy)1);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(pqAboutDialog->sizePolicy().hasHeightForWidth());
    pqAboutDialog->setSizePolicy(sizePolicy);
    gridLayout = new QGridLayout(pqAboutDialog);
    gridLayout->setSpacing(6);
    gridLayout->setMargin(8);
    gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
    label = new QLabel(pqAboutDialog);
    label->setObjectName(QString::fromUtf8("label"));
    QSizePolicy sizePolicy1((QSizePolicy::Policy)0, (QSizePolicy::Policy)0);
    sizePolicy1.setHorizontalStretch(0);
    sizePolicy1.setVerticalStretch(0);
    sizePolicy1.setHeightForWidth(label->sizePolicy().hasHeightForWidth());
    label->setSizePolicy(sizePolicy1);
    label->setPixmap(QPixmap(QString::fromUtf8(":/pqClient/ParaQLogo.png")));

    gridLayout->addWidget(label, 0, 0, 1, 1);

    hboxLayout = new QHBoxLayout();
    hboxLayout->setSpacing(6);
    hboxLayout->setMargin(0);
    hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
    spacerItem = new QSpacerItem(131, 31, QSizePolicy::Expanding, QSizePolicy::Minimum);

    hboxLayout->addItem(spacerItem);

    okButton = new QPushButton(pqAboutDialog);
    okButton->setObjectName(QString::fromUtf8("okButton"));

    hboxLayout->addWidget(okButton);


    gridLayout->addLayout(hboxLayout, 1, 0, 1, 1);

    retranslateUi(pqAboutDialog);
    QObject::connect(okButton, SIGNAL(clicked()), pqAboutDialog, SLOT(accept()));

    QMetaObject::connectSlotsByName(pqAboutDialog);
    } // setupUi

    void retranslateUi(QDialog *pqAboutDialog)
    {
    pqAboutDialog->setWindowTitle(QApplication::translate("pqAboutDialog", "Dialog", 0, QApplication::UnicodeUTF8));
    label->setText(QApplication::translate("pqAboutDialog", "", 0, QApplication::UnicodeUTF8));
    okButton->setText(QApplication::translate("pqAboutDialog", "OK", 0, QApplication::UnicodeUTF8));
    Q_UNUSED(pqAboutDialog);
    } // retranslateUi

};

namespace Ui {
    class pqAboutDialog: public Ui_pqAboutDialog {};
} // namespace Ui

#endif // PQABOUT_H
