#ifndef PQFILEDIALOG_H
#define PQFILEDIALOG_H

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
#include <QtGui/QListView>
#include <QtGui/QPushButton>
#include <QtGui/QToolButton>
#include <QtGui/QTreeView>

class Ui_pqFileDialog
{
public:
    QGridLayout *gridLayout;
    QHBoxLayout *hboxLayout;
    QLabel *label_3;
    QComboBox *parents;
    QToolButton *navigateUp;
    QTreeView *files;
    QListView *favorites;
    QGridLayout *gridLayout1;
    QLabel *label_2;
    QComboBox *fileType;
    QPushButton *buttonOk;
    QLabel *label;
    QLineEdit *fileName;
    QPushButton *buttonCancel;

    void setupUi(QDialog *pqFileDialog)
    {
    pqFileDialog->setObjectName(QString::fromUtf8("pqFileDialog"));
    pqFileDialog->resize(QSize(559, 357).expandedTo(pqFileDialog->minimumSizeHint()));
    gridLayout = new QGridLayout(pqFileDialog);
    gridLayout->setSpacing(6);
    gridLayout->setMargin(8);
    gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
    hboxLayout = new QHBoxLayout();
    hboxLayout->setSpacing(6);
    hboxLayout->setMargin(0);
    hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
    label_3 = new QLabel(pqFileDialog);
    label_3->setObjectName(QString::fromUtf8("label_3"));
    QSizePolicy sizePolicy((QSizePolicy::Policy)1, (QSizePolicy::Policy)5);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(label_3->sizePolicy().hasHeightForWidth());
    label_3->setSizePolicy(sizePolicy);

    hboxLayout->addWidget(label_3);

    parents = new QComboBox(pqFileDialog);
    parents->setObjectName(QString::fromUtf8("parents"));
    QSizePolicy sizePolicy1((QSizePolicy::Policy)7, (QSizePolicy::Policy)0);
    sizePolicy1.setHorizontalStretch(0);
    sizePolicy1.setVerticalStretch(0);
    sizePolicy1.setHeightForWidth(parents->sizePolicy().hasHeightForWidth());
    parents->setSizePolicy(sizePolicy1);

    hboxLayout->addWidget(parents);

    navigateUp = new QToolButton(pqFileDialog);
    navigateUp->setObjectName(QString::fromUtf8("navigateUp"));
    navigateUp->setAutoRaise(true);

    hboxLayout->addWidget(navigateUp);


    gridLayout->addLayout(hboxLayout, 0, 1, 1, 1);

    files = new QTreeView(pqFileDialog);
    files->setObjectName(QString::fromUtf8("files"));
    QSizePolicy sizePolicy2((QSizePolicy::Policy)7, (QSizePolicy::Policy)7);
    sizePolicy2.setHorizontalStretch(2);
    sizePolicy2.setVerticalStretch(0);
    sizePolicy2.setHeightForWidth(files->sizePolicy().hasHeightForWidth());
    files->setSizePolicy(sizePolicy2);

    gridLayout->addWidget(files, 1, 1, 1, 1);

    favorites = new QListView(pqFileDialog);
    favorites->setObjectName(QString::fromUtf8("favorites"));
    QSizePolicy sizePolicy3((QSizePolicy::Policy)7, (QSizePolicy::Policy)7);
    sizePolicy3.setHorizontalStretch(1);
    sizePolicy3.setVerticalStretch(0);
    sizePolicy3.setHeightForWidth(favorites->sizePolicy().hasHeightForWidth());
    favorites->setSizePolicy(sizePolicy3);

    gridLayout->addWidget(favorites, 0, 0, 2, 1);

    gridLayout1 = new QGridLayout();
    gridLayout1->setSpacing(6);
    gridLayout1->setMargin(0);
    gridLayout1->setObjectName(QString::fromUtf8("gridLayout1"));
    label_2 = new QLabel(pqFileDialog);
    label_2->setObjectName(QString::fromUtf8("label_2"));

    gridLayout1->addWidget(label_2, 0, 0, 1, 1);

    fileType = new QComboBox(pqFileDialog);
    fileType->setObjectName(QString::fromUtf8("fileType"));

    gridLayout1->addWidget(fileType, 1, 1, 1, 1);

    buttonOk = new QPushButton(pqFileDialog);
    buttonOk->setObjectName(QString::fromUtf8("buttonOk"));

    gridLayout1->addWidget(buttonOk, 0, 2, 1, 1);

    label = new QLabel(pqFileDialog);
    label->setObjectName(QString::fromUtf8("label"));

    gridLayout1->addWidget(label, 1, 0, 1, 1);

    fileName = new QLineEdit(pqFileDialog);
    fileName->setObjectName(QString::fromUtf8("fileName"));

    gridLayout1->addWidget(fileName, 0, 1, 1, 1);

    buttonCancel = new QPushButton(pqFileDialog);
    buttonCancel->setObjectName(QString::fromUtf8("buttonCancel"));

    gridLayout1->addWidget(buttonCancel, 1, 2, 1, 1);


    gridLayout->addLayout(gridLayout1, 2, 1, 1, 1);

    retranslateUi(pqFileDialog);
    QObject::connect(buttonOk, SIGNAL(clicked()), pqFileDialog, SLOT(accept()));
    QObject::connect(buttonCancel, SIGNAL(clicked()), pqFileDialog, SLOT(reject()));

    QMetaObject::connectSlotsByName(pqFileDialog);
    } // setupUi

    void retranslateUi(QDialog *pqFileDialog)
    {
    pqFileDialog->setWindowTitle(QApplication::translate("pqFileDialog", "Dialog", 0, QApplication::UnicodeUTF8));
    label_3->setText(QApplication::translate("pqFileDialog", "Look in:", 0, QApplication::UnicodeUTF8));
    navigateUp->setText(QApplication::translate("pqFileDialog", "", 0, QApplication::UnicodeUTF8));
    label_2->setText(QApplication::translate("pqFileDialog", "File name:", 0, QApplication::UnicodeUTF8));
    buttonOk->setText(QApplication::translate("pqFileDialog", "OK", 0, QApplication::UnicodeUTF8));
    label->setText(QApplication::translate("pqFileDialog", "Files of type:", 0, QApplication::UnicodeUTF8));
    buttonCancel->setText(QApplication::translate("pqFileDialog", "Cancel", 0, QApplication::UnicodeUTF8));
    Q_UNUSED(pqFileDialog);
    } // retranslateUi

};

namespace Ui {
    class pqFileDialog: public Ui_pqFileDialog {};
} // namespace Ui

#endif // PQFILEDIALOG_H
