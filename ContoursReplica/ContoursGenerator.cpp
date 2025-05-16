#include "ContoursGenerator.h"
#include "DrawOperations.h"
#include <opencv2/opencv.hpp>
#include <opencv2/ximgproc.hpp>
#include <qpainter.h>
#include <qfile.h>
#include <qfiledialog.h>
#include <QProgressDialog>
#include "RandomGenerator.h"
#include <quuid.h>
#include <filesystem>
#include <windows.h>
#include "Strings.h"

ContoursGenerator::ContoursGenerator(QWidget* parent)
	: QMainWindow(parent)
	, ui(new Ui::ContoursGeneratorClass())
{
	ui->setupUi(this);

	initConnections();

	OnChangeMode();
}

ContoursGenerator::~ContoursGenerator()
{
	delete ui;
}

GenerationParams ContoursGenerator::getUIParams()
{
	GenerationParams params{};
	if (ui) {
		params.height = ui->spinBox_Width->value();
		params.width = ui->spinBox_Height->value();
		params.dpi = ui->spinBox_dpi->value();
		params.Xmul = ui->doubleSpinBox_Xmul->value();
		params.Ymul = ui->doubleSpinBox_Ymul->value();

		int minMul = ui->spinBox_TotalMulMin->value();
		int maxMul = ui->spinBox_TotalMulMax->value();
		params.mul = RandomGenerator::instance().getRandomInt(minMul, maxMul);

		params.generateWells = ui->groupBox_Wells->isChecked();
		params.numOfWells = ui->spinBox_Wells->value();
		params.generateIsolines = ui->groupBox_Contours->isChecked();

		int minDensity = ui->spinBox_MinDensity->value();
		int maxDensity = ui->spinBox_MaxDensity->value();
		params.contoursDensity = RandomGenerator::instance().getRandomInt(minDensity, maxDensity);

		double minThickness = ui->doubleSpinBox_MinThickness->value();
		double maxThickness = ui->doubleSpinBox_MaxThickness->value();
		params.contoursThickness = RandomGenerator::instance().getRandomFloat(static_cast<float>(minThickness), static_cast<float>(maxThickness));

		params.fillContours = ui->groupBox_Fill->isChecked();
		params.fillMode = getFillMode();
		params.drawValues = ui->groupBox_DrawValues->isChecked();
		params.saveValuesToFile = ui->checkBox_saveValuesToFile->isChecked();
		params.textDistance = ui->spinBox_TextDistance->value();

		int minTextSize = ui->spinBox_TextMinSize->value();
		int maxTextSize = ui->spinBox_TextMaxSize->value();
		params.textSize = RandomGenerator::instance().getRandomInt(minTextSize, maxTextSize);

		params.mode = getGenMode();
	}
	return params;
}

void ContoursGenerator::initConnections()
{
	connect(ui->pushButton_Generate, &QPushButton::pressed, this, &ContoursGenerator::OnGenerateImage);
	connect(ui->pushButton_256, &QPushButton::pressed, this, &ContoursGenerator::setSize<256>);
	connect(ui->pushButton_512, &QPushButton::pressed, this, &ContoursGenerator::setSize<512>);
	connect(ui->pushButton_1024, &QPushButton::pressed, this, &ContoursGenerator::setSize<1024>);
	connect(ui->pushButton_2048, &QPushButton::pressed, this, &ContoursGenerator::setSize<2048>);
	connect(ui->checkBox_ShowMask, &QCheckBox::stateChanged, this, &ContoursGenerator::OnUpdateImage);
	connect(ui->pushButton_Save, &QPushButton::pressed, this, &ContoursGenerator::OnSaveImage);
	connect(ui->pushButton_GenerateBatch, &QPushButton::pressed, this, &ContoursGenerator::OnSaveBatch);
	connect(ui->radioButton_method1, &QRadioButton::toggled, this, &ContoursGenerator::OnChangeMode);
	connect(ui->radioButton_method2, &QRadioButton::toggled, this, &ContoursGenerator::OnChangeMode);

	connectRange(ui->spinBox_TotalMulMin, ui->spinBox_TotalMulMax);
	connectRange(ui->spinBox_TextMinSize, ui->spinBox_TextMaxSize);
	connectRange(ui->doubleSpinBox_MinThickness, ui->doubleSpinBox_MaxThickness);
	connectRange(ui->spinBox_MinDensity, ui->spinBox_MaxDensity);
}

void ContoursGenerator::OnGenerateImage()
{
	GenImg genImg = generateImage();
	m_generatedImage = genImg.image;
	m_generatedMask = genImg.mask;

	OnUpdateImage();
}

void ContoursGenerator::OnUpdateImage()
{
	if (ui->checkBox_ShowMask->isChecked())
	{
		ui->label_Image->setPixmap(m_generatedMask);
	}
	else
	{
		ui->label_Image->setPixmap(m_generatedImage);
	}
}

void ContoursGenerator::OnSaveImage()
{
	if (m_generatedImage.isNull() || m_generatedMask.isNull())
	{
		return;
	}

	QString folderName = QFileDialog::getExistingDirectory(this);
	if (folderName.isEmpty())
	{
		return;
	}

	saveImage(folderName, m_generatedImage, m_generatedMask);
}

void ContoursGenerator::saveImage(const QString& folderPath, const QPixmap& img, const QPixmap& mask)
{
	QDir().mkpath(folderPath + "/images");
	QDir().mkpath(folderPath + "/masks");

	QString baseName;
	int index = 1;
	QString imageFileName, maskFileName;
	do
	{
		baseName = QString::number(index);
		imageFileName = folderPath + "/images/" + baseName + ".jpg";
		maskFileName = folderPath + "/masks/" + baseName + ".jpg";
		index++;
	} while (QFile::exists(imageFileName) || QFile::exists(maskFileName));

	if (!img.save(imageFileName, "JPG"))
	{
		return;
	}
	if (!mask.save(maskFileName, "JPG"))
	{
		QFile::remove(imageFileName);
		return;
	}
}

void ContoursGenerator::saveImageSplit(const QString& folderPath, const GenImg& gen)
{
	int baseSize = 256;
	int width = gen.image.width();
	int height = gen.image.height();
	int numX = width / baseSize;
	int numY = height / baseSize;

	for (int i = 0; i < numX; ++i)
	{
		for (int j = 0; j < numY; ++j)
		{
			QRect rect(i * baseSize, j * baseSize, baseSize, baseSize);
			QPixmap img = gen.image.copy(rect);
			QPixmap mask = gen.mask.copy(rect);
			saveImage(folderPath, img, mask);
		}
	}
}

GenerationMode ContoursGenerator::getGenMode()
{
	if (ui) {
		if (ui->radioButton_method1->isChecked()) {
			return GenerationMode::python;
		}
	}
	return GenerationMode::legacy;
}

FillMode ContoursGenerator::getFillMode()
{
	if (ui) {
		if (ui->radioButton_FillRandom->isChecked()) {
			return FillMode::random;
		}
	}
	return FillMode::standard;
}

void ContoursGenerator::OnSaveBatch()
{
	QString folderName = QFileDialog::getExistingDirectory(this);
	if (folderName.isEmpty())
	{
		return;
	}

	// show progress dialog
	QProgressDialog progress("Generating images...", "Abort", 0, ui->spinBox_BatchSize->value(), this);
	progress.setWindowModality(Qt::WindowModal);
	progress.setWindowFlags(progress.windowFlags() & ~Qt::WindowContextHelpButtonHint);

	int batchSize = ui->spinBox_BatchSize->value();
	for (int i = 0; i < batchSize; ++i)
	{
		if (progress.wasCanceled())
		{
			break;
		}
		GenImg generation = generateImage();
		saveImageSplit(folderName, generation);
		progress.setValue(i);
	}
	progress.setValue(batchSize);
}

void ContoursGenerator::OnChangeMode()
{
	GenerationMode mode = getGenMode();

	if (mode == GenerationMode::python) {
		ui->spinBox_TextDistance->setEnabled(false);
		ui->spinBox_dpi->setEnabled(true);
		ui->label_width->setText(str_width);
		ui->label_height->setText(str_height);
		ui->groupBox_PerlinNoise->setVisible(false);
		ui->spinBox_MinDensity->setVisible(true);
		ui->spinBox_MaxDensity->setVisible(true);
		ui->label_MinDensity->setVisible(true);
		ui->label_MaxDensity->setVisible(true);
	}
	else if (mode == GenerationMode::legacy) {
		ui->spinBox_TextDistance->setEnabled(true);
		ui->spinBox_dpi->setEnabled(false);
		ui->label_width->setText(str_width_pixels);
		ui->label_height->setText(str_height_pixels);
		ui->groupBox_PerlinNoise->setVisible(true);
		ui->spinBox_MinDensity->setVisible(false);
		ui->spinBox_MaxDensity->setVisible(false);
		ui->label_MinDensity->setVisible(false);
		ui->label_MaxDensity->setVisible(false);
	}

	resize(width(), 1);
}

bool run_python_script_silently(const std::string& command)
{
	STARTUPINFOA si = { sizeof(STARTUPINFOA) };
	PROCESS_INFORMATION pi;
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;

	std::string cmd = "cmd.exe /C " + command;

	BOOL success = CreateProcessA(
		NULL,
		cmd.data(),   // Command line
		NULL, NULL,   // Process and thread security attributes
		FALSE,        // Handle inheritance
		CREATE_NO_WINDOW, // Creation flags
		NULL, NULL,   // Environment and current directory
		&si, &pi      // Startup and process info
	);

	if (!success) {
		return false;
	}

	WaitForSingleObject(pi.hProcess, INFINITE);

	DWORD exit_code = 0;
	GetExitCodeProcess(pi.hProcess, &exit_code);

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return exit_code == 0;
}

GenImg ContoursGenerator::generateImage()
{
	GenerationParams params = getUIParams();

	if (params.mode == GenerationMode::python) {
		return _generateImage_python();
	}
	else {
		return _generateImage_legacy();
	}
}

GenImg ContoursGenerator::_generateImage_legacy()
{
	GenerationParams params = getUIParams();

	cv::Mat isolines; // isolines mat
	cv::Mat mask; // mask mat
	QPixmap pixIso; // visual representation pixmap
	QPixmap pixMask; // mask representation pixmap

	int cropSize = 1;

	if (params.generateIsolines) {
		isolines = ContoursOperations::generateIsolines(params);

		mask = cv::Scalar(255) - isolines;

		// apply thinning
		cv::Mat thinned;
		cv::ximgproc::thinning(mask, thinned, cv::ximgproc::THINNING_GUOHALL);

		// crop by 1 pixel
		cv::Rect cropRect(cropSize, cropSize, thinned.cols - 2 * cropSize, thinned.rows - 2 * cropSize);
		thinned = thinned(cropRect);

		// Find contours
		std::vector<Contour> contours;
		ContoursOperations::findContours(thinned, contours);

		cv::Mat contours_mat = cv::Mat::zeros(thinned.size(), CV_8UC1);
		for (size_t i = 0; i < contours.size(); i++) {
			const Contour& c = contours[i];
			cv::Scalar color = cv::Scalar(255, 255, 255);
			for (size_t j = 0; j < c.points.size(); ++j) {
				contours_mat.at<uchar>(c.points[j]) = c.value;
			}
		}

		// Find depth
		ContoursOperations::findDepth(contours_mat, contours);

		// Depth mat
		cv::Mat depthMat = cv::Mat::zeros(thinned.size(), CV_8UC1);
		for (size_t i = 0; i < contours.size(); i++) {
			const Contour& c = contours[i];
			for (size_t j = 0; j < c.points.size(); ++j) {
				depthMat.at<uchar>(c.points[j]) = c.depth + 1;
			}
		}

		// Draw contours
		cv::Mat drawing = params.fillContours ? cv::Mat::zeros(thinned.size(), CV_8UC3) : cv::Mat(thinned.size(), CV_8UC3, cv::Scalar(255, 255, 255));
		for (size_t i = 0; i < contours.size(); i++) {
			for (size_t j = 0; j < contours[i].points.size(); ++j) {
				cv::Scalar color = contours[i].isClosed ? cv::Scalar(75, 75, 75) : cv::Scalar(150, 100, 150);
				drawing.at<cv::Vec3b>(contours[i].points[j]) = cv::Vec3b(color[0], color[1], color[2]);
			}
		}

		if (params.fillContours) {
			// Fill areas
			ContoursOperations::fillContours(contours_mat, contours, drawing, params.fillMode);
		}

		// Inpaint contours on drawing
		cv::Mat maskInpaint = cv::Mat::zeros(thinned.size(), CV_8UC1);
		for (size_t i = 0; i < contours.size(); i++) {
			const Contour& c = contours[i];
			for (size_t j = 0; j < c.points.size(); ++j) {
				maskInpaint.at<uchar>(c.points[j]) = 255;
			}
		}

		// Inpaint
		cv::inpaint(drawing, maskInpaint, drawing, 3, cv::INPAINT_TELEA);

		pixIso = utils::cvMat2Pixmap(drawing);

		// Draw contours
		float thickness = params.contoursThickness;
		QFont font;
		font.setPointSize(params.textSize);
		{
			QPainter painter(&pixIso);
			for (const auto& contour : contours) {
				if (params.drawValues) {
					DrawOperations::drawContourValues(painter, contour, thickness, QColor(Qt::black), font, params.textDistance, params.saveValuesToFile);
				}
				else {
					DrawOperations::drawContour(painter, contour, QColor(Qt::black), thickness);
				}
			}
		}

		// Draw mask
		mask = cv::Mat::zeros(params.height, params.width, CV_8UC1);
		pixMask = utils::cvMat2Pixmap(mask);
		{
			QPainter painter(&pixMask);
			for (const auto& contour : contours) {
				DrawOperations::drawContour(painter, contour, QColor(Qt::white), thickness);
			}
		}
	}
	else {
		mask = cv::Mat::zeros(params.height, params.width, CV_8UC1);
		isolines = mask.clone() + cv::Scalar(255);
		pixIso = utils::cvMat2Pixmap(isolines);
		pixMask = utils::cvMat2Pixmap(mask);
	}

	if (params.generateWells) {
		WellParams wellParams = getUIWellParams();
		for (int i = 0; i < params.numOfWells; ++i) {
			DrawOperations::drawRandomWell(pixIso, wellParams);
		}
	}

	// inpaint cropped pixels
	cv::Mat pixIsoUncropped = utils::QPixmap2cvMat(pixIso, false);
	cv::Mat maskUncropped = cv::Mat::zeros(pixIsoUncropped.size(), CV_8UC1);
	// enlarge by 1 pixel
	cv::copyMakeBorder(pixIsoUncropped, pixIsoUncropped, cropSize, cropSize, cropSize, cropSize, cv::BORDER_CONSTANT, cv::Scalar(255, 255, 255));
	cv::copyMakeBorder(maskUncropped, maskUncropped, cropSize, cropSize, cropSize, cropSize, cv::BORDER_CONSTANT, cv::Scalar(255));
	cv::inpaint(pixIsoUncropped, maskUncropped, pixIsoUncropped, 3, cv::INPAINT_TELEA);

	QPixmap pixIsoResult = utils::cvMat2Pixmap(pixIsoUncropped);

	GenImg result{ pixIsoResult, pixMask };
	return result;
}

GenImg ContoursGenerator::_generateImage_python()
{
	GenerationParams params = getUIParams();

	cv::Mat isolines; // isolines mat
	cv::Mat mask; // mask mat
	QPixmap pixIso; // visual representation pixmap

	int cropSize = 1;

	std::string temp_name = QUuid::createUuid().toString().toStdString();
	std::string temp_mask_name = QUuid::createUuid().toString().toStdString();

	auto remove = [](std::string& str, char ch) {
		str.erase(std::remove(str.begin(), str.end(), ch), str.end());
		};

	auto remove_guid = [&](std::string& str) {
		remove(str, '{');
		remove(str, '}');
		remove(str, '"');
		remove(str, '\'');
		remove(str, '-');
		};
	remove_guid(temp_name);
	remove_guid(temp_mask_name);

	const std::string output_path = temp_name + "_out.png";
	const std::string output_mask_path = temp_mask_name + "_out.png";

	try {
		const std::string python_command = "py -3.10 generate_contours.py --output " + output_path + " --output_mask " + output_mask_path
			+ " --w " + std::to_string(params.width)
			+ " --h " + std::to_string(params.height)
			+ " --dpi " + std::to_string(params.dpi)
			+ " --draw_isolines " + std::to_string(params.generateIsolines)
			+ " --fill_isolines " + std::to_string(params.fillContours)
			+ " --draw_values " + std::to_string(params.drawValues)
			+ " --text_size " + std::to_string(params.textSize)
			+ " --contours_density " + std::to_string(params.contoursDensity)
			+ " --contours_thickness " + std::to_string(params.contoursThickness)
			+ " --fill_mode " + std::to_string(static_cast<int>(params.fillMode))
			;
		bool success = run_python_script_silently(python_command);

		if (!success) {
			throw std::runtime_error("Python script execution failed.");
		}

		cv::Mat result = cv::imread(output_path);
		cv::Mat result_mask = cv::imread(output_mask_path);
		if (result.empty() || result_mask.empty()) {
			throw std::runtime_error("Failed to read output image.");
		}
	}
	catch (...) {
		if (std::filesystem::exists(output_path)) {
			std::filesystem::remove(output_path);
		}
		if (std::filesystem::exists(output_mask_path)) {
			std::filesystem::remove(output_mask_path);
		}
		throw;
	}

	QPixmap pixIsolines(QString::fromStdString(output_path));

	if (params.generateWells) {
		WellParams wellParams = getUIWellParams();
		for (int i = 0; i < params.numOfWells; ++i) {
			DrawOperations::drawRandomWell(pixIsolines, wellParams);
		}
	}

	QPixmap pixMask(QString::fromStdString(output_mask_path));

	if (std::filesystem::exists(output_path)) {
		std::filesystem::remove(output_path);
	}
	if (std::filesystem::exists(output_mask_path)) {
		std::filesystem::remove(output_mask_path);
	}

	GenImg result { pixIsolines, pixMask };
	return result;
}

WellParams ContoursGenerator::getUIWellParams()
{
	WellParams params{};
	if (ui)
	{
		params.drawText = ui->groupBox_Wellname->isChecked();
		params.fontSize = ui->spinBox_wellFontSize->value();
		params.radius = ui->spinBox_WellRadius->value();
		params.offset = ui->spinBox_WellnameOffset->value();
		params.outline = ui->spinBox_WellOutline->value();
	}

	params.color = RandomGenerator::instance().getRandomColor();

	return params;
}

QPixmap utils::cvMat2Pixmap(const cv::Mat& input)
{
	QImage image;
	if (input.channels() == 3)
	{
		image = QImage((uchar*)input.data, input.cols, input.rows, input.step, QImage::Format_BGR888);
	}
	else if (input.channels() == 1)
	{
		image = QImage((uchar*)input.data, input.cols, input.rows, input.step, QImage::Format_Grayscale8);
	}
	QPixmap cpy = QPixmap::fromImage(image);
	return cpy;
}

cv::Mat utils::QPixmap2cvMat(const QPixmap& in, bool grayscale)
{
	QImage im = in.toImage();
	if (grayscale)
	{
		im.convertTo(QImage::Format_Grayscale8);
		return cv::Mat(im.height(), im.width(), CV_8UC1, const_cast<uchar*>(im.bits()), im.bytesPerLine()).clone();
	}
	else
	{
		im.convertTo(QImage::Format_BGR888);
		return cv::Mat(im.height(), im.width(), CV_8UC3, const_cast<uchar*>(im.bits()), im.bytesPerLine()).clone();
	}
}

template<int size>
inline void ContoursGenerator::setSize()
{
	ui->spinBox_Height->setValue(size);
	ui->spinBox_Width->setValue(size);
}