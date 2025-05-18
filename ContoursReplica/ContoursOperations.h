#pragma once
#include <opencv2/opencv.hpp>

struct Contour
{
    int index;
    double value;
    bool isClosed;
    int depth;
    std::vector<cv::Point> points;
    cv::Rect boundingRect;
};

class ColorScaler
{
public:
    ColorScaler(double min, double max, const cv::Scalar& minColor, const cv::Scalar& maxColor);
    ColorScaler() {}
    void init(double min, double max, const cv::Scalar& minColor, const cv::Scalar& maxColor);
    cv::Scalar getColor(double value) const;
protected:
    double m_min;
    double m_max;
    cv::Scalar m_minColor;
    cv::Scalar m_maxColor;
};

enum class Direction
{
    TOP,
    TOP_RIGHT,
    RIGHT,
    BOTTOM_RIGHT,
    DOWN,
    BOTTOM_LEFT,
    LEFT,
    TOP_LEFT,
    NONE
};

enum class GenerationMode
{
    legacy,
    python
};

enum class FillMode
{
    standard,
    random
};

struct GenerationParams
{
    int width, height; // image size
    int dpi; // render dpi
    double Xmul, Ymul; // multipliers for X and Y for Perlin noise
    int mul; // general multiplier for Perlin noise
    bool generateWells; // generate wells
    int numOfWells; // number of wells
    bool generateIsolines; // generate isolines
    int contoursDensity; // contours density
    float contoursThickness; // contours thickness
    bool fillContours; // fill contours with color
    FillMode fillMode; // fill color scheme
    bool drawValues; // draw values on isolines
    bool saveValuesToFile; // save contour values to separate files
    bool saveBoundingBoxesToFile; // save contour values' bounding boxes to file
    int textDistance; // minimal distance between texts on isolines
    int textSize; // font size
    GenerationMode mode;
};

namespace ContoursOperations
{
    cv::Mat generateIsolines(const GenerationParams& params);
    void findContours(const cv::Mat& img, std::vector<Contour>& contours);
    void extractContour(int x_start, int y_start, cv::Mat& img, std::vector<cv::Point>& contour);
    Direction getDirection(cv::Point prev, cv::Point next);
    std::vector<cv::Point> getOrder(cv::Point pt, Direction direction);
    // Find depth of each contour
    void findDepth(cv::Mat& img, std::vector<Contour>& contours);
    void fillContours(cv::Mat& contoursMat, const std::vector<Contour>& contours, cv::Mat& drawing, FillMode fillMode);
};

