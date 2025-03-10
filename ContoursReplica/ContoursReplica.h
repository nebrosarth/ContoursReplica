#pragma once

#include <QtWidgets/QWidget>
#include "ui_ContoursReplica.h"

QT_BEGIN_NAMESPACE
namespace Ui { class ContoursReplicaClass; };
QT_END_NAMESPACE

class ContoursReplica : public QWidget
{
    Q_OBJECT

public:
    ContoursReplica(QWidget *parent = nullptr);
    ~ContoursReplica();

private:
    Ui::ContoursReplicaClass *ui;
};
