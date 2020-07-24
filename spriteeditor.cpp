#include "spriteeditor.h"
#include "crc32.h"
#include "invisible.h"

#include <QFile>
#include <QDir>
#include <QDebug>
#include <QtEndian>
#include <cstring>
#include <QFileInfo>


#define GAMEDATA_DAT_LENGTH 95044834
#define PNG_HEADER_LENGTH 8
#define PNG_TYPE_COUNT 21

const char *pngHeader = "\x89\x50\x4e\x47\x0d\x0a\x1a\x0a";

enum PNGTypes:uint32_t {PNG_IHDR=1229472850, PNG_PLTE=1347179589, PNG_IDAT=1229209940,
                        PNG_IEND=1229278788, PNG_bKGD=1649100612, PNG_cHRM=1665684045,
                        PNG_dSIG=1683179847, PNG_eXIf=1700284774, PNG_gAMA=1732332865,
                        PNG_hIST=1749635924, PNG_iCCP=1766015824, PNG_iTXt=1767135348,
                        PNG_pHYs=1883789683, PNG_sBIT=1933723988, PNG_sPLT=1934642260,
                        PNG_sRGB=1934772034, PNG_sTER=1934902610, PNG_tEXt=1950701684,
                        PNG_tIME=1950960965, PNG_tRNS=1951551059, PNG_zTXt=2052348020};

const uint32_t pngTypes[] = {PNG_IHDR, PNG_PLTE, PNG_IDAT,
                             PNG_IEND, PNG_bKGD, PNG_cHRM,
                             PNG_dSIG, PNG_eXIf, PNG_gAMA,
                             PNG_hIST, PNG_iCCP, PNG_iTXt,
                             PNG_pHYs, PNG_sBIT, PNG_sPLT,
                             PNG_sRGB, PNG_sTER, PNG_tEXt,
                             PNG_tIME, PNG_tRNS, PNG_zTXt};

#define MINIMUM_PAD_AMOUNT 15
#define IEND_SIZE 12

SpriteEditor::SpriteEditor()
{

}


enum SpriteEditorReturn SpriteEditor::unpackSprites(QString inputFilename, QString outputDirectory, bool overwriteFiles)
{
    QFile inputFile(inputFilename);
    if (!inputFile.open(QIODevice::ReadOnly)) {
        return SER_ERROR_INPUT_FILE;
    }
    QByteArray inputFileArray = inputFile.readAll();
    inputFile.close();
    if (inputFileArray.length() != GAMEDATA_DAT_LENGTH) {
        return SER_ERROR_INPUT_FILE;
    }

    QDir directory(outputDirectory);
    if (!directory.exists()) {
        return SER_ERROR_OUTPUT_DIR;
    }

    findPNGs(&inputFileArray);
    if (pngLocations.size() == 0) {
        return SER_ERROR_INPUT_FILE;
    }

    // Checks if any of the images currently exists, if they do, require overwriting.
    if (!overwriteFiles) {
        for (uint32_t i = 0; i < pngLocations.size(); i++) {
            QString filename = directory.absoluteFilePath("image");
            filename += QString::number(i) + ".png";
            QFileInfo testFile(filename);
            if (testFile.exists()) {
                return SER_ERROR_OVERWRITE;
            }
        }
    }

    // Save PNGs.
    char *data = inputFileArray.data();
    for (uint32_t i = 0; i < pngLocations.size(); i++) {
        QString filename = directory.absoluteFilePath("image");
        filename += QString::number(i) + ".png";
        QFile outputFile(filename);
        if (outputFile.open(QIODevice::WriteOnly)) {
            int bytesWritten = outputFile.write(data + pngLocations[i], pngLengths[i]);
            if (bytesWritten < pngLengths[i]) {
                outputFile.close();
                return SER_ERROR_PNG_OUTPUT;
            }
            outputFile.close();
        }
    }

    return SER_SUCCESS;
}


enum SpriteEditorReturn SpriteEditor::packSprites(QString inputFilename, QString outputFilename, QString inputDirectory, QString *errorExtra)
{
    QFile inputFile(inputFilename);
    if (!inputFile.open(QIODevice::ReadOnly)) {
        return SER_ERROR_INPUT_FILE;
    }
    QByteArray inputFileArray = inputFile.readAll();
    inputFile.close();
    if (inputFileArray.length() != GAMEDATA_DAT_LENGTH) {
        return SER_ERROR_INPUT_FILE;
    }

    QDir directory(inputDirectory);
    if (!directory.exists()) {
        return SER_ERROR_INPUT_DIR;
    }

    findPNGs(&inputFileArray);
    if (pngLocations.size() == 0) {
        return SER_ERROR_INPUT_FILE;
    }

    char *outputData = (char*) malloc(inputFileArray.size());
    if (outputData == NULL) {
        return SER_ERROR_INTERNAL;
    }
    memcpy(outputData, inputFileArray.data(), inputFileArray.size());

    QStringList filters;
    QStringList fileList = directory.entryList(filters, QDir::Files, QDir::NoSort);
    for (int i = 0; i < fileList.size(); i++) {
        QString filename = fileList[i];
        if (filename.startsWith("image") && filename.endsWith(".png")) {
            QString numberString = filename;
            numberString.remove("image");
            numberString.remove(".png");
            bool validNumber;
            uint32_t index = numberString.toUInt(&validNumber);
            if (validNumber && (index < pngLocations.size())) {
                qDebug() << filename;
                QString fullFilename = directory.absoluteFilePath(filename);
                QFile inputPNG(fullFilename);
                if (!inputPNG.open(QIODevice::ReadOnly)) {
                    free(outputData);
                    *errorExtra = filename;
                    return SER_ERROR_INPUT_PNG;
                }
                QByteArray pngArray = inputPNG.readAll();
                if ((pngArray.size() == pngLengths[index]) || (pngArray.size() < pngLengths[index] - MINIMUM_PAD_AMOUNT)) {
                    char *paddedPNG = getPaddedPNG(&pngArray, pngLengths[index]);
                    if (paddedPNG == NULL) {
                        free(outputData);
                        *errorExtra = filename;
                        return SER_ERROR_INPUT_PNG;
                    }
                    memcpy(outputData + pngLocations[index], paddedPNG, pngLengths[index]);
                    free(paddedPNG);
                } else {
                    free(outputData);
                    *errorExtra = filename;
                    return SER_ERROR_PNG_SIZE;
                }
            }
        }
    }

    QFile outputFile(outputFilename);
    if (outputFile.open(QIODevice::WriteOnly)) {
        int bytesWritten = outputFile.write(outputData, GAMEDATA_DAT_LENGTH);
        if (bytesWritten < GAMEDATA_DAT_LENGTH) {
            outputFile.close();
            free(outputData);
            return SER_ERROR_DAT_OUTPUT;
        }
        outputFile.close();
    } else {
        free(outputData);
        return SER_ERROR_DAT_OUTPUT;
    }

    free(outputData);
    return SER_SUCCESS;
}


char *SpriteEditor::getPaddedPNG(QByteArray *array, int length)
{
    char *output = (char*) malloc(length);
    if (output == NULL) {
        return NULL;
    }
    if (array->size() == length) {
        memcpy(output, array->data(), length);
        return output;
    }
    int index = array->size() - IEND_SIZE;
    memcpy(output, array->data(), index);
    int overallPaddingAmount = length - array->size();
    uint32_t internalPaddingAmount = overallPaddingAmount - 12;
    uint32_t internalBigEndian = qToBigEndian<quint32>(internalPaddingAmount);
    memcpy(output + index, &internalBigEndian, sizeof(uint32_t));
    index += sizeof(uint32_t);
    output[index] = 't';
    index++;
    output[index] = 'E';
    index++;
    output[index] = 'X';
    index++;
    output[index] = 't';
    index++;
    output[index] = 'a';
    index++;
    output[index] = 0;
    index++;
    for (uint32_t i = 0; i < internalPaddingAmount - 2; i++) {
        output[index] = 'a';
        index++;
    }
    // Calculate CRC.
    uint32_t crcLittleEndian = crc32::calc_crc_32((unsigned char*) output + array->size() - IEND_SIZE, overallPaddingAmount - 4);
    uint32_t crcBigEndian = qToBigEndian<quint32>(crcLittleEndian);
    memcpy(output + index, &crcBigEndian, sizeof(uint32_t));
    // Append IEND.
    memcpy(output + length - IEND_SIZE, array->data() + array->size() - IEND_SIZE, IEND_SIZE);
    return output;
}

void SpriteEditor::findPNGs(QByteArray *array)
{
    pngLocations.clear();
    pngLengths.clear();
    int index = 0;
    int pngStart;
    int pngLength;
    bool hasFoundPNG = true;
    bool shouldContinue = true;
    while (shouldContinue) {
        shouldContinue = findPNG(array, index, &hasFoundPNG, &pngStart, &pngLength);
        if (hasFoundPNG) {
            pngLocations.push_back(pngStart);
            pngLengths.push_back(pngLength);
        }
        index = pngStart + pngLength;
    }
}


// Returns whether we should continue searching.
// Always returns a outputIndex and outputLength, which tell us where to skip to.
// These are a valid PNG if hasFoundPNG == true, otherwise just tell us where to skip to.
bool SpriteEditor::findPNG(QByteArray *array, int startIndex, bool *hasFoundPNG, int *outputIndex, int *outputLength)
{
    if (startIndex > array->length()) {
        assert(false);
    }

    int headerIndex = array->indexOf(pngHeader, startIndex);
    if (headerIndex == -1) {
        *hasFoundPNG = false;
        *outputIndex = array->length();
        *outputLength = 0;
        return false;
    }

    int index = headerIndex + PNG_HEADER_LENGTH;
    bool isFirst = true;
    uint32_t chunkType;
    int chunkLength;
    while (true) {
        bool isValidChunk = processChunk(array, index, &chunkType, &chunkLength);
        if (!isValidChunk) {
            *hasFoundPNG = false;
            *outputIndex = headerIndex;
            *outputLength = PNG_HEADER_LENGTH;
            return true;
        }
        //qDebug() << "    Found chunk: " << chunkType;
        if (isFirst) {
            if (chunkType == PNG_IHDR) {
                isFirst = false;
            } else {
                *hasFoundPNG = false;
                *outputIndex = headerIndex;
                *outputLength = PNG_HEADER_LENGTH;
                return true;
            }
        }
        index += chunkLength;
        if (chunkType == PNG_IEND) {
            *hasFoundPNG = true;
            *outputIndex = headerIndex;
            *outputLength = index - headerIndex;
            return true;
        }
    }
}

// Returns whether we received a valid chunk.
//
bool SpriteEditor::processChunk(QByteArray *array, int startIndex, uint32_t *outputType, int *outputLength)
{
    if (startIndex > array->length() - 8) {
        return false;
    }
    char *data = array->data();

    uint32_t length;
    uint32_t type;
    memcpy(&length, data + startIndex, sizeof(uint32_t));
    memcpy(&type, data + startIndex + sizeof(uint32_t), sizeof(uint32_t));
    length = qFromBigEndian<quint32>(length);
    type = qFromBigEndian<quint32>(type);

    // Check very basic validity.
    bool isValidType = false;
    for (int i = 0; i < PNG_TYPE_COUNT; i++) {
        if (type == pngTypes[i]) {
            isValidType = true;
            break;
        }
    }
    //qDebug() << "    length: " << length << " type: " << type;
    if (!isValidType) {
        *outputType = 0;
        *outputLength = 0;
        return false;
    }
    // Check length for validity. Adds crc, length, type.
    if (startIndex + (sizeof(uint32_t) * 3) + length > (uint32_t) array->length()) {
        *outputType = 0;
        *outputLength = 0;
        return false;
    }
    // Return.
    *outputType = type;
    *outputLength = length + (sizeof(uint32_t) * 3);
    return true;
}


char *SpriteEditor::getPaddedXorPNG(uint8_t *originalPNG, uint8_t *xorArray, int xorLength, int outputLength)
{
    char *output = (char*) malloc(outputLength);
    if (output == NULL) {
        return NULL;
    }

    assert(outputLength >= xorLength + IEND_SIZE + MINIMUM_PAD_AMOUNT);

    // Xor and copy data.
    for (int i = 0; i < xorLength; i++) {
        output[i] = originalPNG[i] ^ xorArray[i];
    }

    int index = xorLength;
    int overallPaddingAmount = outputLength - xorLength - IEND_SIZE;
    uint32_t internalPaddingAmount = overallPaddingAmount - 12;
    uint32_t internalBigEndian = qToBigEndian<quint32>(internalPaddingAmount);
    memcpy(output + index, &internalBigEndian, sizeof(uint32_t));
    index += sizeof(uint32_t);
    output[index] = 't';
    index++;
    output[index] = 'E';
    index++;
    output[index] = 'X';
    index++;
    output[index] = 't';
    index++;
    output[index] = 'a';
    index++;
    output[index] = 0;
    index++;
    for (uint32_t i = 0; i < internalPaddingAmount - 2; i++) {
        output[index] = 'a';
        index++;
    }
    // Calculate CRC.
    uint32_t crcLittleEndian = crc32::calc_crc_32((unsigned char*) output + xorLength, overallPaddingAmount - 4);
    uint32_t crcBigEndian = qToBigEndian<quint32>(crcLittleEndian);
    memcpy(output + index, &crcBigEndian, sizeof(uint32_t));
    // Append IEND.
    memcpy(output + outputLength - IEND_SIZE, originalPNG + outputLength - IEND_SIZE, IEND_SIZE);
    return output;
}



enum SpriteEditorReturn SpriteEditor::createInvisible(QString inputFilename, QString outputFilename)
{
    QFile inputFile(inputFilename);
    if (!inputFile.open(QIODevice::ReadOnly)) {
        return SER_ERROR_INPUT_FILE;
    }
    QByteArray inputFileArray = inputFile.readAll();
    inputFile.close();
    if (inputFileArray.length() != GAMEDATA_DAT_LENGTH) {
        return SER_ERROR_INPUT_FILE;
    }

    findPNGs(&inputFileArray);
    if (pngLocations.size() == 0) {
        return SER_ERROR_INPUT_FILE;
    }

    char *outputData = (char*) malloc(inputFileArray.size());
    if (outputData == NULL) {
        return SER_ERROR_INTERNAL;
    }
    memcpy(outputData, inputFileArray.data(), inputFileArray.size());

    for (int i = 0; i < invisibleCount; i++) {
        uint32_t index = invisibleIndices[i];
        char *originalPNG = inputFileArray.data() + pngLocations[index];
        uint8_t *xorArray = invisibleData[i];
        int xorLength = invisibleLengths[i];
        int outputLength = pngLengths[index];
        char *paddedPNG = getPaddedXorPNG((uint8_t *) originalPNG, xorArray, xorLength, outputLength);

        if (paddedPNG == NULL) {
            free(outputData);
            return SER_ERROR_INTERNAL;
        }
        memcpy(outputData + pngLocations[index], paddedPNG, pngLengths[index]);
        free(paddedPNG);
    }

    QFile outputFile(outputFilename);
    if (outputFile.open(QIODevice::WriteOnly)) {
        int bytesWritten = outputFile.write(outputData, GAMEDATA_DAT_LENGTH);
        if (bytesWritten < GAMEDATA_DAT_LENGTH) {
            outputFile.close();
            free(outputData);
            return SER_ERROR_DAT_OUTPUT;
        }
        outputFile.close();
    } else {
        free(outputData);
        return SER_ERROR_DAT_OUTPUT;
    }

    free(outputData);
    return SER_SUCCESS;
}


enum SpriteEditorReturn SpriteEditor::createInvisibleTrails(QString inputFilename, QString outputFilename)
{
    QFile inputFile(inputFilename);
    if (!inputFile.open(QIODevice::ReadOnly)) {
        return SER_ERROR_INPUT_FILE;
    }
    QByteArray inputFileArray = inputFile.readAll();
    inputFile.close();
    if (inputFileArray.length() != GAMEDATA_DAT_LENGTH) {
        return SER_ERROR_INPUT_FILE;
    }

    findPNGs(&inputFileArray);
    if (pngLocations.size() == 0) {
        return SER_ERROR_INPUT_FILE;
    }

    char *outputData = (char*) malloc(inputFileArray.size());
    if (outputData == NULL) {
        return SER_ERROR_INTERNAL;
    }
    memcpy(outputData, inputFileArray.data(), inputFileArray.size());

    for (int i = 0; i < invisibleTrailsCount; i++) {
        uint32_t index = invisibleTrailsIndices[i];
        char *originalPNG = inputFileArray.data() + pngLocations[index];
        uint8_t *xorArray = invisibleTrailsData[i];
        int xorLength = invisibleTrailsLengths[i];
        int outputLength = pngLengths[index];
        char *paddedPNG = getPaddedXorPNG((uint8_t *) originalPNG, xorArray, xorLength, outputLength);

        if (paddedPNG == NULL) {
            free(outputData);
            return SER_ERROR_INTERNAL;
        }
        memcpy(outputData + pngLocations[index], paddedPNG, pngLengths[index]);
        free(paddedPNG);
    }

    QFile outputFile(outputFilename);
    if (outputFile.open(QIODevice::WriteOnly)) {
        int bytesWritten = outputFile.write(outputData, GAMEDATA_DAT_LENGTH);
        if (bytesWritten < GAMEDATA_DAT_LENGTH) {
            outputFile.close();
            free(outputData);
            return SER_ERROR_DAT_OUTPUT;
        }
        outputFile.close();
    } else {
        free(outputData);
        return SER_ERROR_DAT_OUTPUT;
    }

    free(outputData);
    return SER_SUCCESS;
}
