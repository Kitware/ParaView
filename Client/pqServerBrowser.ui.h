#ifndef PQSERVERBROWSER_H
#define PQSERVERBROWSER_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QComboBox>
#include <QtGui/QDialog>
#include <QtGui/QGridLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QPushButton>
#include <QtGui/QSpacerItem>
#include <QtGui/QSpinBox>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>

class Ui_pqServerBrowser
{
public:
    QWidget *verticalLayout;
    QVBoxLayout *vboxLayout;
    QHBoxLayout *hboxLayout;
    QLabel *label;
    QComboBox *serverType;
    QGridLayout *gridLayout;
    QLabel *label_2;
    QLabel *label_3;
    QLineEdit *hostName;
    QSpinBox *portNumber;
    QHBoxLayout *hboxLayout1;
    QSpacerItem *spacerItem;
    QPushButton *okButton;
    QPushButton *cancelButton;

    void setupUi(QDialog *pqServerBrowser)
    {
    pqServerBrowser->setObjectName(QString::fromUtf8("pqServerBrowser"));
    pqServerBrowser->resize(QSize(398, 230).expandedTo(pqServerBrowser->minimumSizeHint()));
    verticalLayout = new QWidget(pqServerBrowser);
    verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
    verticalLayout->setGeometry(QRect(9, 9, 381, 160));
    vboxLayout = new QVBoxLayout(verticalLayout);
    vboxLayout->setSpacing(6);
    vboxLayout->setMargin(0);
    vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
    hboxLayout = new QHBoxLayout();
    hboxLayout->setSpacing(6);
    hboxLayout->setMargin(0);
    hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
    label = new QLabel(verticalLayout);
    label->setObjectName(QString::fromUtf8("label"));
    label->setCursor(QCursor(static_cast<Qt::CursorShape>(0)));

    hboxLayout->addWidget(label);

    serverType = new QComboBox(verticalLayout);
    serverType->setObjectName(QString::fromUtf8("serverType"));
    QSizePolicy sizePolicy((QSizePolicy::Policy)7, (QSizePolicy::Policy)0);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(serverType->sizePolicy().hasHeightForWidth());
    serverType->setSizePolicy(sizePolicy);

    hboxLayout->addWidget(serverType);


    vboxLayout->addLayout(hboxLayout);

    gridLayout = new QGridLayout();
    gridLayout->setSpacing(6);
    gridLayout->setMargin(0);
    gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
    label_2 = new QLabel(verticalLayout);
    label_2->setObjectName(QString::fromUtf8("label_2"));

    gridLayout->addWidget(label_2, 0, 0, 1, 1);

    label_3 = new QLabel(verticalLayout);
    label_3->setObjectName(QString::fromUtf8("label_3"));

    gridLayout->addWidget(label_3, 1, 0, 1, 1);

    hostName = new QLineEdit(verticalLayout);
    hostName->setObjectName(QString::fromUtf8("hostName"));

    gridLayout->addWidget(hostName, 0, 1, 1, 1);

    portNumber = new QSpinBox(verticalLayout);
    portNumber->setObjectName(QString::fromUtf8("portNumber"));
    portNumber->setMaximum(65535);
    portNumber->setMinimum(1);
    portNumber->setValue(11111);

    gridLayout->addWidget(portNumber, 1, 1, 1, 1);


    vboxLayout->addLayout(gridLayout);

    hboxLayout1 = new QHBoxLayout();
    hboxLayout1->setSpacing(6);
    hboxLayout1->setMargin(0);
    hboxLayout1->setObjectName(QString::fromUtf8("hboxLayout1"));
    spacerItem = new QSpacerItem(131, 31, QSizePolicy::Expanding, QSizePolicy::Minimum);

    hboxLayout1->addItem(spacerItem);

    okButton = new QPushButton(verticalLayout);
    okButton->setObjectName(QString::fromUtf8("okButton"));

    hboxLayout1->addWidget(okButton);

    cancelButton = new QPushButton(verticalLayout);
    cancelButton->setObjectName(QString::fromUtf8("cancelButton"));

    hboxLayout1->addWidget(cancelButton);


    vboxLayout->addLayout(hboxLayout1);

    retranslateUi(pqServerBrowser);
    QObject::connect(okButton, SIGNAL(clicked()), pqServerBrowser, SLOT(accept()));
    QObject::connect(cancelButton, SIGNAL(clicked()), pqServerBrowser, SLOT(reject()));

    QMetaObject::connectSlotsByName(pqServerBrowser);
    } // setupUi

    void retranslateUi(QDialog *pqServerBrowser)
    {
    pqServerBrowser->setWindowTitle(QApplication::translate("pqServerBrowser", "Dialog", 0, QApplication::UnicodeUTF8));
    label->setText(QApplication::translate("pqServerBrowser", "Choose Server:", 0, QApplication::UnicodeUTF8));
    label_2->setText(QApplication::translate("pqServerBrowser", "Host", 0, QApplication::UnicodeUTF8));
    label_3->setText(QApplication::translate("pqServerBrowser", "Port", 0, QApplication::UnicodeUTF8));
    hostName->setText(QApplication::translate("pqServerBrowser", "localhost", 0, QApplication::UnicodeUTF8));
    okButton->setText(QApplication::translate("pqServerBrowser", "OK", 0, QApplication::UnicodeUTF8));
    cancelButton->setText(QApplication::translate("pqServerBrowser", "Cancel", 0, QApplication::UnicodeUTF8));
    Q_UNUSED(pqServerBrowser);
    } // retranslateUi

};

namespace Ui {
    class pqServerBrowser: public Ui_pqServerBrowser {};
} // namespace Ui

#endif // PQSERVERBROWSER_H
