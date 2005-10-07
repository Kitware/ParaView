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
#include <QtGui/QWidget>

class Ui_pqServerBrowser
{
public:
    QWidget *layoutWidget_2;
    QHBoxLayout *hboxLayout;
    QLabel *label_4;
    QComboBox *serverType;
    QWidget *layoutWidget_3;
    QHBoxLayout *hboxLayout1;
    QSpacerItem *spacerItem;
    QPushButton *okButton;
    QPushButton *cancelButton;
    QWidget *layoutWidget;
    QGridLayout *gridLayout;
    QLabel *label_2_2;
    QLabel *label_3_2;
    QLineEdit *hostName;
    QSpinBox *portNumber;

    void setupUi(QDialog *pqServerBrowser)
    {
    pqServerBrowser->setObjectName(QString::fromUtf8("pqServerBrowser"));
    pqServerBrowser->resize(QSize(261, 145).expandedTo(pqServerBrowser->minimumSizeHint()));
    layoutWidget_2 = new QWidget(pqServerBrowser);
    layoutWidget_2->setObjectName(QString::fromUtf8("layoutWidget_2"));
    layoutWidget_2->setGeometry(QRect(10, 10, 241, 20));
    hboxLayout = new QHBoxLayout(layoutWidget_2);
    hboxLayout->setSpacing(6);
    hboxLayout->setMargin(0);
    hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
    label_4 = new QLabel(layoutWidget_2);
    label_4->setObjectName(QString::fromUtf8("label_4"));
    label_4->setCursor(QCursor(static_cast<Qt::CursorShape>(0)));

    hboxLayout->addWidget(label_4);

    serverType = new QComboBox(layoutWidget_2);
    serverType->setObjectName(QString::fromUtf8("serverType"));
    QSizePolicy sizePolicy((QSizePolicy::Policy)7, (QSizePolicy::Policy)0);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(serverType->sizePolicy().hasHeightForWidth());
    serverType->setSizePolicy(sizePolicy);

    hboxLayout->addWidget(serverType);

    layoutWidget_3 = new QWidget(pqServerBrowser);
    layoutWidget_3->setObjectName(QString::fromUtf8("layoutWidget_3"));
    layoutWidget_3->setGeometry(QRect(10, 100, 241, 33));
    hboxLayout1 = new QHBoxLayout(layoutWidget_3);
    hboxLayout1->setSpacing(6);
    hboxLayout1->setMargin(0);
    hboxLayout1->setObjectName(QString::fromUtf8("hboxLayout1"));
    spacerItem = new QSpacerItem(131, 31, QSizePolicy::Expanding, QSizePolicy::Minimum);

    hboxLayout1->addItem(spacerItem);

    okButton = new QPushButton(layoutWidget_3);
    okButton->setObjectName(QString::fromUtf8("okButton"));

    hboxLayout1->addWidget(okButton);

    cancelButton = new QPushButton(layoutWidget_3);
    cancelButton->setObjectName(QString::fromUtf8("cancelButton"));

    hboxLayout1->addWidget(cancelButton);

    layoutWidget = new QWidget(pqServerBrowser);
    layoutWidget->setObjectName(QString::fromUtf8("layoutWidget"));
    layoutWidget->setGeometry(QRect(10, 40, 241, 43));
    gridLayout = new QGridLayout(layoutWidget);
    gridLayout->setSpacing(6);
    gridLayout->setMargin(0);
    gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
    label_2_2 = new QLabel(layoutWidget);
    label_2_2->setObjectName(QString::fromUtf8("label_2_2"));

    gridLayout->addWidget(label_2_2, 0, 0, 1, 1);

    label_3_2 = new QLabel(layoutWidget);
    label_3_2->setObjectName(QString::fromUtf8("label_3_2"));

    gridLayout->addWidget(label_3_2, 1, 0, 1, 1);

    hostName = new QLineEdit(layoutWidget);
    hostName->setObjectName(QString::fromUtf8("hostName"));

    gridLayout->addWidget(hostName, 0, 1, 1, 1);

    portNumber = new QSpinBox(layoutWidget);
    portNumber->setObjectName(QString::fromUtf8("portNumber"));
    portNumber->setMaximum(65535);
    portNumber->setMinimum(1);
    portNumber->setValue(11111);

    gridLayout->addWidget(portNumber, 1, 1, 1, 1);

    retranslateUi(pqServerBrowser);
    QObject::connect(okButton, SIGNAL(clicked()), pqServerBrowser, SLOT(accept()));
    QObject::connect(cancelButton, SIGNAL(clicked()), pqServerBrowser, SLOT(reject()));

    QMetaObject::connectSlotsByName(pqServerBrowser);
    } // setupUi

    void retranslateUi(QDialog *pqServerBrowser)
    {
    pqServerBrowser->setWindowTitle(QApplication::translate("pqServerBrowser", "Dialog", 0, QApplication::UnicodeUTF8));
    label_4->setText(QApplication::translate("pqServerBrowser", "Server Type:", 0, QApplication::UnicodeUTF8));
    okButton->setText(QApplication::translate("pqServerBrowser", "OK", 0, QApplication::UnicodeUTF8));
    cancelButton->setText(QApplication::translate("pqServerBrowser", "Cancel", 0, QApplication::UnicodeUTF8));
    label_2_2->setText(QApplication::translate("pqServerBrowser", "Host", 0, QApplication::UnicodeUTF8));
    label_3_2->setText(QApplication::translate("pqServerBrowser", "Port", 0, QApplication::UnicodeUTF8));
    hostName->setText(QApplication::translate("pqServerBrowser", "localhost", 0, QApplication::UnicodeUTF8));
    Q_UNUSED(pqServerBrowser);
    } // retranslateUi

};

namespace Ui {
    class pqServerBrowser: public Ui_pqServerBrowser {};
} // namespace Ui

#endif // PQSERVERBROWSER_H
