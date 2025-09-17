#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "MultiSniffer.h"
#include "PacketModel.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    MultiSniffer    sniffer;
    PacketModel    *model;

private slots:
    void on_startButton_clicked();
    void on_stopButton_clicked();
    void on_resumeButton_clicked();
    void onTableSelectionChanged(const QModelIndex &current, const QModelIndex &previous);
};

#endif // MAINWINDOW_H
