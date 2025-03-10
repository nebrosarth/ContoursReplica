#pragma once
#include "ContoursOperations.h"

#include <QtWidgets/QWidget>
#include "ui_ContoursGenerator.h"

QT_BEGIN_NAMESPACE
namespace Ui { class ContoursGeneratorClass; };
QT_END_NAMESPACE

struct GenerationParams;
struct WellParams;

struct GenImg
{
    QPixmap image;
    QPixmap mask;
};

namespace utils
{
    QPixmap cvMat2Pixmap(const cv::Mat& input);
    cv::Mat QPixmap2cvMat(const QPixmap& in, bool grayscale);
}

class ContoursGenerator : public QMainWindow
{
    Q_OBJECT

public:
    ContoursGenerator(QWidget *parent = nullptr);
    ~ContoursGenerator();

protected slots:
    void OnGenerateImage();
    void OnUpdateImage();
    void OnSaveImage();
    void OnSaveBatch();

protected:
    void initConnections();
    GenerationParams getUIParams();
    GenImg generateImage();
    WellParams getUIWellParams();
    void saveImage(const QString& folderPath, const QPixmap& img, const QPixmap& mask);
    void saveImageSplit(const QString& folderPath, const GenImg& gen);

    template<int size>
    void setSize(); // set image size

private:
    Ui::ContoursGeneratorClass *ui;
    QPixmap m_generatedImage;
    QPixmap m_generatedMask;
};
