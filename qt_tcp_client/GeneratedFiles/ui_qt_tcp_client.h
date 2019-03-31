/********************************************************************************
** Form generated from reading UI file 'qt_tcp_client.ui'
**
** Created by: Qt User Interface Compiler version 5.12.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_QT_TCP_CLIENT_H
#define UI_QT_TCP_CLIENT_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_qt_tcp_clientClass
{
public:

    void setupUi(QWidget *qt_tcp_clientClass)
    {
        if (qt_tcp_clientClass->objectName().isEmpty())
            qt_tcp_clientClass->setObjectName(QString::fromUtf8("qt_tcp_clientClass"));
        qt_tcp_clientClass->resize(600, 400);

        retranslateUi(qt_tcp_clientClass);

        QMetaObject::connectSlotsByName(qt_tcp_clientClass);
    } // setupUi

    void retranslateUi(QWidget *qt_tcp_clientClass)
    {
        qt_tcp_clientClass->setWindowTitle(QApplication::translate("qt_tcp_clientClass", "qt_tcp_client", nullptr));
    } // retranslateUi

};

namespace Ui {
    class qt_tcp_clientClass: public Ui_qt_tcp_clientClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_QT_TCP_CLIENT_H
