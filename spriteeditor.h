#ifndef SPRITEEDITOR_H
#define SPRITEEDITOR_H

#include <QString>
#include <vector>
enum SpriteEditorReturn {SER_SUCCESS, SER_ERROR_INPUT_FILE,
                         SER_ERROR_OUTPUT_DIR, SER_ERROR_PNG_OUTPUT,
                         SER_ERROR_INPUT_DIR, SER_ERROR_OVERWRITE,
                         SER_ERROR_INTERNAL, SER_ERROR_INPUT_PNG,
                         SER_ERROR_PNG_SIZE, SER_ERROR_DAT_OUTPUT};



class SpriteEditor
{
public:
    SpriteEditor();
    enum SpriteEditorReturn unpackSprites(QString inputFilename, QString outputDirectory, bool overwriteFiles);
    enum SpriteEditorReturn packSprites(QString inputFilename, QString outputFilename, QString inputDirectory, QString *errorExtra);
    enum SpriteEditorReturn createInvisible(QString inputFilename, QString outputFilename);
    enum SpriteEditorReturn createInvisibleTrails(QString inputFilename, QString outputFilename);


    bool findPNG(QByteArray *array, int startIndex, bool *hasFoundPNG, int *outputIndex, int *outputLength);
    bool processChunk(QByteArray *array, int startIndex, uint32_t *outputType, int *outputLength);
    void findPNGs(QByteArray *array);
    char *getPaddedPNG(QByteArray *array, int length);
    char *getPaddedXorPNG(uint8_t *originalPNG, uint8_t *xorArray, int xorLength, int outputLength);

private:
    std::vector<int> pngLocations;
    std::vector<int> pngLengths;
};

#endif // SPRITEEDITOR_H
