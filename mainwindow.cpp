#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    inputFilename = QString();
    outputFilename = QString();
    inputDirectory = QString();
    outputDirectory = QString();
    spriteEditor = SpriteEditor();
}

MainWindow::~MainWindow()
{
    delete ui;
}




void MainWindow::on_inputFileButton_clicked()
{
    QString result = QFileDialog::getOpenFileName(this, tr("Select input file"), NULL, tr("Dat Files (*.dat);;All Files (*)"));
    if (!result.isEmpty()) {
        inputFilename = result;
        ui->inputFilenameLabel->setText(inputFilename);
    }
    ui->statusLabel->setText("");

}
void MainWindow::on_outputFileButton_clicked()
{
    QString result = QFileDialog::getSaveFileName(this, tr("Save File"), NULL, tr("Dat Files (*.dat)"));
    if (!result.isEmpty()) {
        if (!result.endsWith(".dat", Qt::CaseInsensitive)) {
            result += ".dat";
        }
        outputFilename = result;
        ui->outputFilenameLabel->setText(outputFilename);
    }
    ui->statusLabel->setText("");
}

void MainWindow::on_actionExit_triggered()
{
    close();
}

void MainWindow::on_actionAbout_triggered()
{
    QString text = "Includes code from d265f27 - MIT - https://github.com/d265f27\n";
    text += "Lammert Bies - MIT - https://github.com/lammertb/libcrc\n";
    text += "QT - LGPL/GPL - https://qt.io";
    QMessageBox::information(this, tr("Sprite Loader"), text);
}


void MainWindow::on_inputDirectoryButton_clicked()
{
    QString result = QFileDialog::getExistingDirectory(this, tr("Select sprite load directory"), NULL, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!result.isEmpty()) {
        inputDirectory = result;
        ui->inputDirectoryLabel->setText(inputDirectory);
    }
    ui->statusLabel->setText("");
}

void MainWindow::on_outputDirectoryButton_clicked()
{
    QString result = QFileDialog::getExistingDirectory(this, tr("Select sprite output directory"), NULL, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!result.isEmpty()) {
        outputDirectory = result;
        ui->outputDirectoryLabel->setText(outputDirectory);
    }
    ui->statusLabel->setText("");
}

void MainWindow::on_unpackSpritesButton_clicked()
{
    if (inputFilename.isEmpty()) {
        ui->statusLabel->setText("Error: No input filename set.");
        return;
    }
    if (outputDirectory.isEmpty()) {
        ui->statusLabel->setText("Error: No output directory set.");
        return;
    }
    enum SpriteEditorReturn result = spriteEditor.unpackSprites(inputFilename, outputDirectory, ui->allowOverwritingCheckBox->isChecked());
    reportResult(result, "Sucessfully unpacked sprites.", NULL);
}

void MainWindow::on_packSpritesButton_clicked()
{
    if (inputFilename.isEmpty()) {
        ui->statusLabel->setText("Error: No input filename set.");
        return;
    }
    if (outputFilename.isEmpty()) {
        ui->statusLabel->setText("Error: No output filename set.");
        return;
    }
    if (inputDirectory.isEmpty()) {
        ui->statusLabel->setText("Error: No input directory set.");
        return;
    }
    QString errorExtra;
    enum SpriteEditorReturn result = spriteEditor.packSprites(inputFilename, outputFilename, inputDirectory, &errorExtra);
    reportResult(result, "Sucessfully packed sprites.", &errorExtra);
}

void MainWindow::on_invisibleTrailsButton_clicked()
{
    if (inputFilename.isEmpty()) {
        ui->statusLabel->setText("Error: No input filename set.");
        return;
    }
    if (outputFilename.isEmpty()) {
        ui->statusLabel->setText("Error: No output filename set.");
        return;
    }
    enum SpriteEditorReturn result = spriteEditor.createInvisibleTrails(inputFilename, outputFilename);
    reportResult(result, "Sucessfully created invisible with trails dat.", NULL);
}

void MainWindow::on_invisibleButton_clicked()
{
    if (inputFilename.isEmpty()) {
        ui->statusLabel->setText("Error: No input filename set.");
        return;
    }
    if (outputFilename.isEmpty()) {
        ui->statusLabel->setText("Error: No output filename set.");
        return;
    }
    enum SpriteEditorReturn result = spriteEditor.createInvisible(inputFilename, outputFilename);
    reportResult(result, "Sucessfully created invisible dat.", NULL);
}


void MainWindow::reportResult(enum SpriteEditorReturn result, const char *string, QString *errorExtra)
{
    if (result == SER_SUCCESS) {
        ui->statusLabel->setText(string);
    } else if (result == SER_ERROR_INPUT_FILE) {
        ui->statusLabel->setText("Error: Unable to read input file.");
    } else if (result == SER_ERROR_OUTPUT_DIR) {
        ui->statusLabel->setText("Error: Unable to open output directory.");
    } else if (result == SER_ERROR_PNG_OUTPUT) {
        ui->statusLabel->setText("Error: Failed to save an image.");
    } else if (result == SER_ERROR_INPUT_DIR) {
        ui->statusLabel->setText("Error: Unable to open input directory.");
    } else if (result == SER_ERROR_OVERWRITE) {
        ui->statusLabel->setText("Error: Would overwrite files.");
    } else if (result == SER_ERROR_INTERNAL) {
        ui->statusLabel->setText("Error: Internal error.");
    } else if (result == SER_ERROR_INPUT_PNG) {
        ui->statusLabel->setText("Error: Unable to open image: " + *errorExtra);
    } else if (result == SER_ERROR_PNG_SIZE) {
        ui->statusLabel->setText("Error: Image too large: " + *errorExtra);
    } else if (result == SER_ERROR_DAT_OUTPUT) {
        ui->statusLabel->setText("Error: Unable to write .dat file.");
    }

}
