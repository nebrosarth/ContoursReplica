#pragma once
#include "ContoursOperations.h"

#include <QtWidgets/QWidget>
#include "ui_ContoursGenerator.h"

QT_BEGIN_NAMESPACE
namespace Ui { class ContoursGeneratorClass; };
QT_END_NAMESPACE

struct GenerationParams;
struct WellParams;
struct BoundingBox;

struct GenImg
{
    QPixmap image;
    QPixmap mask;
    std::vector<BoundingBox> bboxes;
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
    void OnChangeMode();

protected:
    void initConnections();
    GenerationParams getUIParams();
    GenImg generateImage();
    GenImg _generateImage_legacy();
    GenImg _generateImage_python();
    WellParams getUIWellParams();
    void saveImage(const QString& folderPath, const QPixmap& img, const QPixmap& mask, const std::vector<BoundingBox>& bboxes);
    void saveImageSplit(const QString& folderPath, const GenImg& gen);
    GenerationMode getGenMode();
    FillMode getFillMode();

    template<int size>
    void setSize(); // set image size

    template <typename SpinBoxT>
    void connectRange(SpinBoxT* minWidget, SpinBoxT* maxWidget)
    {
        using ValueT = decltype(minWidget->value());

        auto sync = [this, minWidget, maxWidget]() {
            QObject* obj = this->sender();
            auto senderSpin = qobject_cast<SpinBoxT*>(obj);
            if (!senderSpin) return;

            ValueT minVal = minWidget->value();
            ValueT maxVal = maxWidget->value();

            QSignalBlocker blockMin(minWidget);
            QSignalBlocker blockMax(maxWidget);

            if (senderSpin == minWidget && minVal > maxVal) {
                maxWidget->setValue(minVal);
            }
            else if (senderSpin == maxWidget && maxVal < minVal) {
                minWidget->setValue(maxVal);
            }
            };

        connect(minWidget,
            QOverload<ValueT>::of(&SpinBoxT::valueChanged),
            this, sync);
        connect(maxWidget,
            QOverload<ValueT>::of(&SpinBoxT::valueChanged),
            this, sync);
    }

private:
    Ui::ContoursGeneratorClass *ui;
    QPixmap m_generatedImage;
    QPixmap m_generatedMask;
    std::vector<BoundingBox> m_bboxes;
};
