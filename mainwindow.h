#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "spriteeditor.h"
#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_inputFileButton_clicked();

    void on_actionExit_triggered();

    void on_actionAbout_triggered();

    void on_outputFileButton_clicked();

    void on_inputDirectoryButton_clicked();

    void on_outputDirectoryButton_clicked();

    void on_unpackSpritesButton_clicked();

    void on_packSpritesButton_clicked();

    void on_invisibleTrailsButton_clicked();

    void on_invisibleButton_clicked();

private:
    Ui::MainWindow *ui;
    SpriteEditor spriteEditor;
    void reportResult(enum SpriteEditorReturn result, const char *string, QString *errorExtra);

    QString inputFilename;
    QString outputFilename;
    QString inputDirectory;
    QString outputDirectory;

};
#endif // MAINWINDOW_H
