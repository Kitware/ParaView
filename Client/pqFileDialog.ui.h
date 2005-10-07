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
#include <QtGui/QPushButton>
#include <QtGui/QToolButton>
#include <QtGui/QTreeView>
#include <QtGui/QVBoxLayout>

class Ui_pqFileDialog
{
public:
    QVBoxLayout *vboxLayout;
    QLabel *viewDirectory;
    QHBoxLayout *hboxLayout;
    QLabel *label_3;
    QComboBox *comboBox;
    QToolButton *toolButton;
    QTreeView *treeView;
    QGridLayout *gridLayout;
    QLabel *label_2;
    QComboBox *cbFileType;
    QPushButton *buttonOk;
    QLabel *label;
    QLineEdit *leFileName;
    QPushButton *buttonCancel;

    void setupUi(QDialog *pqFileDialog)
    {
    pqFileDialog->setObjectName(QString::fromUtf8("pqFileDialog"));
    pqFileDialog->resize(QSize(394, 303).expandedTo(pqFileDialog->minimumSizeHint()));
    vboxLayout = new QVBoxLayout(pqFileDialog);
    vboxLayout->setSpacing(6);
    vboxLayout->setMargin(8);
    vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
    viewDirectory = new QLabel(pqFileDialog);
    viewDirectory->setObjectName(QString::fromUtf8("viewDirectory"));

    vboxLayout->addWidget(viewDirectory);

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

    comboBox = new QComboBox(pqFileDialog);
    comboBox->setObjectName(QString::fromUtf8("comboBox"));
    QSizePolicy sizePolicy1((QSizePolicy::Policy)7, (QSizePolicy::Policy)0);
    sizePolicy1.setHorizontalStretch(0);
    sizePolicy1.setVerticalStretch(0);
    sizePolicy1.setHeightForWidth(comboBox->sizePolicy().hasHeightForWidth());
    comboBox->setSizePolicy(sizePolicy1);

    hboxLayout->addWidget(comboBox);

    toolButton = new QToolButton(pqFileDialog);
    toolButton->setObjectName(QString::fromUtf8("toolButton"));
    toolButton->setAutoRaise(true);

    hboxLayout->addWidget(toolButton);


    vboxLayout->addLayout(hboxLayout);

    treeView = new QTreeView(pqFileDialog);
    treeView->setObjectName(QString::fromUtf8("treeView"));

    vboxLayout->addWidget(treeView);

    gridLayout = new QGridLayout();
    gridLayout->setSpacing(6);
    gridLayout->setMargin(0);
    gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
    label_2 = new QLabel(pqFileDialog);
    label_2->setObjectName(QString::fromUtf8("label_2"));

    gridLayout->addWidget(label_2, 0, 0, 1, 1);

    cbFileType = new QComboBox(pqFileDialog);
    cbFileType->setObjectName(QString::fromUtf8("cbFileType"));

    gridLayout->addWidget(cbFileType, 1, 1, 1, 1);

    buttonOk = new QPushButton(pqFileDialog);
    buttonOk->setObjectName(QString::fromUtf8("buttonOk"));

    gridLayout->addWidget(buttonOk, 0, 2, 1, 1);

    label = new QLabel(pqFileDialog);
    label->setObjectName(QString::fromUtf8("label"));

    gridLayout->addWidget(label, 1, 0, 1, 1);

    leFileName = new QLineEdit(pqFileDialog);
    leFileName->setObjectName(QString::fromUtf8("leFileName"));

    gridLayout->addWidget(leFileName, 0, 1, 1, 1);

    buttonCancel = new QPushButton(pqFileDialog);
    buttonCancel->setObjectName(QString::fromUtf8("buttonCancel"));

    gridLayout->addWidget(buttonCancel, 1, 2, 1, 1);


    vboxLayout->addLayout(gridLayout);

    retranslateUi(pqFileDialog);
    QObject::connect(buttonOk, SIGNAL(clicked()), pqFileDialog, SLOT(accept()));
    QObject::connect(buttonCancel, SIGNAL(clicked()), pqFileDialog, SLOT(reject()));

    QMetaObject::connectSlotsByName(pqFileDialog);
    } // setupUi

    void retranslateUi(QDialog *pqFileDialog)
    {
    pqFileDialog->setWindowTitle(QApplication::translate("pqFileDialog", "Dialog", 0, QApplication::UnicodeUTF8));
    viewDirectory->setText(QApplication::translate("pqFileDialog", "c:\\foo\\", 0, QApplication::UnicodeUTF8));
    label_3->setText(QApplication::translate("pqFileDialog", "Look in:", 0, QApplication::UnicodeUTF8));
    toolButton->setText(QApplication::translate("pqFileDialog", "", 0, QApplication::UnicodeUTF8));
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
