#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QItemSelectionModel>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , model(new PacketModel(this))
{
    ui->setupUi(this);
    ui->tableView->setModel(model);
    ui->tableView->horizontalHeader()
        ->setSectionResizeMode(QHeaderView::Stretch);

    // Сигнал от MultiSniffer -> модель
    connect(&sniffer, &MultiSniffer::packetCaptured,
            model,   &PacketModel::appendPacket);

    // Кнопки
    connect(ui->startButton,  &QPushButton::clicked,
            &sniffer,         &MultiSniffer::startSniff);
    connect(ui->stopButton,   &QPushButton::clicked,
            this,             &MainWindow::on_stopButton_clicked);
    connect(ui->resumeButton, &QPushButton::clicked,
            &sniffer,         &MultiSniffer::resumeSniff);

    // Отображение деталей при выборе строки
    auto sel = ui->tableView->selectionModel();
    connect(sel, &QItemSelectionModel::currentRowChanged,
            this, &MainWindow::onTableSelectionChanged);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onTableSelectionChanged(const QModelIndex &current,
                                         const QModelIndex &)
{
    if (!current.isValid()) {
        ui->detailView->clear();
        return;
    }

    auto p = model->packetAt(current.row());
    QString details;
    details  = QString("Время: %1\n")
                  .arg(p.value("timestamp").toDateTime()
                           .toString("HH:mm:ss.zzz"));
    details += QString("Протокол: %1\n")
                   .arg(p.value("protocol").toString());
    details += QString("Источник: %1:%2\n")
                   .arg(p.value("src").toString())
                   .arg(p.value("srcPort").toString());
    details += QString("Назначение: %1:%2\n")
                   .arg(p.value("dst").toString())
                   .arg(p.value("dstPort").toString());
    details += QString("Длина: %1 bytes\n")
                   .arg(p.value("length").toString());

    ui->detailView->setPlainText(details);
}

void MainWindow::on_startButton_clicked()
{
    sniffer.startSniff();
}

void MainWindow::on_stopButton_clicked()
{
    sniffer.stopSniff();
}

void MainWindow::on_resumeButton_clicked()
{
    sniffer.resumeSniff();
}
