#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "pcmstream.h"

#include <QMainWindow>
#include <QAudioOutput>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButton_clicked();

private:
    Ui::MainWindow *ui;
    void initializeAudio(const QAudioDeviceInfo &deviceInfo);


private:
    QScopedPointer<PcmStream> m_generator;
    QScopedPointer<QAudioOutput> m_audioOutput;

};

#endif // MAINWINDOW_H
