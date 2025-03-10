#include "ContoursReplica.h"

ContoursReplica::ContoursReplica(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ContoursReplicaClass())
{
    ui->setupUi(this);
}

ContoursReplica::~ContoursReplica()
{
    delete ui;
}
