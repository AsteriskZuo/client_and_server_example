#pragma once

#include <QtWidgets/QWidget>
#include "ui_qt_tcp_client.h"

class qt_tcp_client : public QWidget
{
	Q_OBJECT

public:
	qt_tcp_client(QWidget *parent = Q_NULLPTR);

private:
	Ui::qt_tcp_clientClass ui;
};
