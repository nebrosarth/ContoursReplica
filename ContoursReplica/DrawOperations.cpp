#include "DrawOperations.h"
#include "RandomGenerator.h"
#include <qpainter.h>
#include "ContoursOperations.h"
#include <qpainterpath.h>
#include <qdir.h>
#include <quuid.h>

void DrawOperations::drawRandomWell(QPixmap& image, const WellParams& params)
{
	int width = image.width();
	int height = image.height();
	int radius = params.radius;

	auto& gen = RandomGenerator::instance();
	QPoint wellPt = gen.getRandomPoint(image.width(), image.height());

	QPainter painter(&image);

	int outline = params.outline;
	if (outline > 0)
	{
		QPen pen = painter.pen();
		pen.setWidth(params.outline);
		painter.setPen(pen);
	}
	else
	{
		painter.setPen(Qt::NoPen);
	}

	painter.setBrush(params.color);
	painter.drawEllipse(wellPt, radius, radius);

	if (params.drawText)
	{
		drawWellTitle(painter, wellPt, params);
	}
}

void DrawOperations::drawWellTitle(QPainter& painter, const QPoint& wellPt, const WellParams& params)
{
	int offset = params.radius + params.offset;
	QPoint textPt(wellPt.x() + offset, wellPt.y() - offset);

	QFont font;
	font.setPointSize(params.fontSize);
	painter.setFont(font);

	painter.setPen(QPen());

	short idWell = RandomGenerator::instance().getRandomInt(999);

	QString idWellStr = QString::number(idWell);

	painter.drawText(textPt, idWellStr);
}

void DrawOperations::drawContourValues(QPainter& painter, const Contour& contour, QColor textColor, const QFont& font, int minTextDistance, bool saveToFile)
{
	painter.setPen(textColor);
	painter.setFont(font);

	QPainterPath clipPath;

	QString contourFolderName;
	QDir globalFolder;
	QDir contourFolder;
	bool deleteFolder = false;
	if (saveToFile)
	{
		deleteFolder = true;

		std::string resultImageGuid = std::to_string((long long)painter.device());
		QString globalFolderName = "tmp/" +  QString::fromStdString(resultImageGuid);  // Глобальная папка по адресу устройства

		// Проверяем, существует ли глобальная папка
		if (!globalFolder.exists(globalFolderName))
		{
			// Если нет, создаем папку с уникальным именем
			if (!globalFolder.mkpath(globalFolderName))
			{
				//qWarning() << "Не удалось создать глобальную папку:" << globalFolderName;
			}
		}

		// Создаем уникальную папку для контура внутри глобальной папки
		contourFolderName = QUuid::createUuid().toString();
		contourFolderName = contourFolderName.mid(1, contourFolderName.length() - 2);  // Убираем фигурные скобки из GUID

		// Папка для контура должна быть внутри глобальной папки, но не дублировать саму себя
		contourFolder = QDir(globalFolderName);  // Убедитесь, что путь не будет ошибочно включать имя глобальной папки
		if (!contourFolder.exists(contourFolderName))
		{
			if (!contourFolder.mkpath(contourFolderName))
			{
				//qWarning() << "Не удалось создать папку для контура:" << contourFolderName;
			}
		}
		contourFolder = QDir(globalFolderName + "/" + contourFolderName);
	}

	// Draw text along the contour points
	// Calc the text rotation angle according to the slope of the line
	// Add rotated bounding rect of text to the clip path

	double minDist = minTextDistance;
	cv::Point prevPt;
	for (size_t i = 0; i < contour.points.size(); i++)
	{
		cv::Point pt1_cv = contour.points[i];
		cv::Point pt2_cv = contour.points[(i + 1) % contour.points.size()];

		if(i != 0)
		{
			double dist = cv::norm(pt1_cv - prevPt);
			if (dist < minDist)
			{
				continue;
			}
			double distEnd = cv::norm(pt1_cv - contour.points.back());
			if (distEnd < minDist)
			{
				break;
			}
		}
		prevPt = pt1_cv;

		QPoint pt1 = { pt1_cv.x, pt1_cv.y };
		QPoint pt2 = { pt2_cv.x, pt2_cv.y };

		QLineF line(pt1, pt2);
		double angle = line.angle();

		if (angle > 180) 
		{
			angle -= 180;
		}

		if (angle > 90)
		{
			angle -= 180;
		}

		angle = 360 - angle;

		QFont rotatedFont = font;
		rotatedFont.setPointSize(10);
		rotatedFont.setBold(true);

		// generate random 4-digit number
		int randomNum = RandomGenerator::instance().getRandomInt(9999);

		QRectF textRect = painter.boundingRect(QRect(), Qt::AlignCenter, QString::number(randomNum));

		QTransform transform;
		transform.translate(pt1.x(), pt1.y());
		transform.rotate(angle);

		QRectF rotatedRect = transform.mapRect(textRect);

		clipPath.addRect(rotatedRect);

		painter.save();
		painter.translate(pt1);
		//painter.translate(-textRect.bottomRight());
		painter.rotate(angle);


		//painter.drawText(textRect, Qt::AlignCenter, QString::number(contour.depth + 1));
		painter.drawText(textRect, Qt::AlignCenter, QString::number(randomNum));
		//painter.drawRect(textRect);
		painter.restore();
		painter.resetTransform();

		// Теперь извлекаем часть изображения, содержащую этот текст
		//QImage resultImage = painter.device().ima;  // Получаем текущее изображение
		QPixmap resultImage = *(QPixmap*)painter.device();

		// Копируем часть изображения с числом
		QImage numberImage = resultImage.toImage().copy(rotatedRect.toRect());

		if (!saveToFile)
			continue;

		// if number doesn't fit into resultImage, then skip it
		if (rotatedRect.x() < 0 || rotatedRect.y() < 0 || rotatedRect.x() + rotatedRect.width() > resultImage.width() || rotatedRect.y() + rotatedRect.height() > resultImage.height())
		{
			continue;
		}

		// Сохраняем это изображение в уникальной папке
		QString fileName = "contour_" + QString::number(randomNum) + ".png";
		QString path = contourFolder.absoluteFilePath(fileName);
		numberImage.save(contourFolder.absoluteFilePath(fileName), "PNG");

		deleteFolder = false;
	}

	if (contourFolder.isEmpty())
	{
		contourFolder.removeRecursively();
	}

	QPainterPath clipInv;
	clipInv.addRect({0,0,INT_MAX,INT_MAX});
	clipInv -= clipPath;

	painter.setClipPath(clipInv);

	drawContour(painter, contour, Qt::black);
}

void DrawOperations::drawContour(QPainter& painter, const Contour& contour, QColor color)
{
	// Draw contour polyline
	painter.setPen(color);
	std::vector<QPoint> pts;
	pts.reserve(contour.points.size());
	for (auto& pt : contour.points)
	{
		pts.emplace_back(pt.x, pt.y);
	}

	painter.drawPolyline(pts.data(), contour.points.size());
}
