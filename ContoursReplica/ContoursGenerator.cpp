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

ContoursGenerator::ContoursGenerator(QWidget* parent)
	: QMainWindow(parent)
	, ui(new Ui::ContoursGeneratorClass())
{
	ui->setupUi(this);

	initConnections();
}

ContoursGenerator::~ContoursGenerator()
{
	delete ui;
}

GenerationParams ContoursGenerator::getUIParams()
{
	GenerationParams params{};
	if (ui)
	{
		params.height = ui->spinBox_Width->value();
		params.width = ui->spinBox_Height->value();
		params.dpi = ui->spinBox_dpi->value();
		params.Xmul = ui->doubleSpinBox_Xmul->value();
		params.Ymul = ui->doubleSpinBox_Ymul->value();
		params.mul = ui->spinBox_mul->value();
		params.generateWells = ui->groupBox_Wells->isChecked();
		params.numOfWells = ui->spinBox_Wells->value();
		params.generateIsolines = ui->groupBox_Contours->isChecked();
		params.fillContours = ui->checkBox_Fill->isChecked();
		params.drawValues = ui->groupBox_DrawValues->isChecked();
		params.saveValuesToFile = ui->checkBox_saveValuesToFile->isChecked();
		params.textDistance = ui->spinBox_TextDistance->value();
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
	int index = 0;
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

bool run_python_script_silently(const std::string& command)
{
	STARTUPINFOA si = { sizeof(STARTUPINFOA) };
	PROCESS_INFORMATION pi;
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;  // скрыть окно

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

	// Дождаться завершения скрипта
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