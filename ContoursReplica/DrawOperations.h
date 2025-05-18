#pragma once
#include <qimage.h>

struct Contour;

struct WellParams
{
	int radius;
	int fontSize;
	int offset;
	bool drawText;
	int outline;
	QColor color;
};

struct BoundingBox
{
	BoundingBox(const QRectF& _bbox, const QString& _value) : bbox(_bbox), value(_value) {}
	QRectF bbox;
	QString value;
};

namespace DrawOperations
{
	void drawRandomWell(QPixmap& image, const WellParams& params);
	void drawWellTitle(QPainter& painter, const QPoint& wellPt, const WellParams& params);
	void drawContourValues(QPainter& painter, const Contour& contour, float width, QColor textColor, const QFont& font, int minTextDistance, bool saveToFile, bool saveBBtoFile, std::vector<BoundingBox>& bbox);
	void drawContour(QPainter& painter, const Contour& contour, QColor color, float width);
};

