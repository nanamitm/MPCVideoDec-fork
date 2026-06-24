#include "MainWindow.h"
#include "MPCVideoDecRegistry.h"
#include "../../filters/transform/MPCVideoDec/MPCVideoDecStatusShare.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QStackedLayout>
#include <QVBoxLayout>

using namespace MPCVideoDecRegistry;

namespace {

Qt::CheckState ArModeToCheckState(DWORD value)
{
	switch (value) {
		case 0: return Qt::Unchecked;
		case 1: return Qt::Checked;
		default: return Qt::PartiallyChecked;
	}
}

DWORD CheckStateToArMode(Qt::CheckState state)
{
	switch (state) {
		case Qt::Unchecked: return 0;
		case Qt::Checked: return 1;
		default: return 2;
	}
}

// Reads MPCVideoDecStatusShare.h's shared block the same way the filter's
// CMPCVideoDecStatusPublisher writes it: a seqlock, retried on a torn read.
bool ReadStatusBlock(MPCVideoDecStatusBlock& out)
{
	HANDLE hMapping = OpenFileMappingW(FILE_MAP_READ, FALSE, MPCVideoDecStatusShareName);
	if (!hMapping) {
		return false;
	}

	auto* block = static_cast<MPCVideoDecStatusBlock*>(MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, sizeof(MPCVideoDecStatusBlock)));
	bool ok = false;
	if (block) {
		for (int attempt = 0; attempt < 8; attempt++) {
			LONG seq1 = block->Sequence;
			if (seq1 & 1) {
				continue;
			}
			MPCVideoDecStatusBlock snapshot = *block;
			LONG seq2 = block->Sequence;
			if (seq1 != seq2) {
				continue;
			}
			if (snapshot.ProcessId != 0) {
				out = snapshot;
				ok = true;
			}
			break;
		}
		UnmapViewOfFile(block);
	}
	CloseHandle(hMapping);
	return ok;
}

} // namespace

MainWindow::MainWindow(QWidget* parent)
	: QWidget(parent)
{
	setWindowTitle(QStringLiteral("MPC Video Decoder Settings (Qt6)"));
	resize(560, 800);

	auto* content = new QWidget;
	auto* contentLayout = new QVBoxLayout(content);
	contentLayout->addWidget(BuildVideoSettingsGroup());
	contentLayout->addWidget(BuildHwAccelerationGroup());
	contentLayout->addWidget(BuildOutputFormatsGroup());
	contentLayout->addWidget(BuildStatusGroup());
	contentLayout->addStretch();

	auto* scrollArea = new QScrollArea;
	scrollArea->setWidget(content);
	scrollArea->setWidgetResizable(true);
	scrollArea->setFrameShape(QFrame::NoFrame);

	auto* buttonRow = new QHBoxLayout;
	auto* resetButton = new QPushButton(QStringLiteral("Reset"));
	auto* reloadButton = new QPushButton(QStringLiteral("Reload"));
	auto* saveButton = new QPushButton(QStringLiteral("Save"));
	buttonRow->addStretch();
	buttonRow->addWidget(resetButton);
	buttonRow->addWidget(reloadButton);
	buttonRow->addWidget(saveButton);

	auto* rootLayout = new QVBoxLayout(this);
	rootLayout->addWidget(scrollArea);
	rootLayout->addLayout(buttonRow);

	connect(resetButton, &QPushButton::clicked, this, &MainWindow::Reset);
	connect(reloadButton, &QPushButton::clicked, this, &MainWindow::Load);
	connect(saveButton, &QPushButton::clicked, this, &MainWindow::Save);

	Load();

	// Same out-of-process status mechanism as the WinUI3 app: the filter
	// has no in-proc COM connection to this process, so the only way to see
	// live status is to poll the shared-memory block it publishes from
	// Transform() while decoding.
	connect(&m_statusTimer, &QTimer::timeout, this, &MainWindow::RefreshStatus);
	m_statusTimer.start(1000);
	RefreshStatus();
}

QWidget* MainWindow::BuildVideoSettingsGroup()
{
	auto* group = new QGroupBox(QStringLiteral("Video settings"));
	auto* form = new QFormLayout(group);

	m_threadNumber = new QComboBox;
	m_threadNumber->addItem(QStringLiteral("Auto"));
	for (int i = 1; i <= 16; i++) {
		m_threadNumber->addItem(QString::number(i));
	}
	form->addRow(QStringLiteral("Number of decoding threads"), m_threadNumber);

	m_scanType = new QComboBox;
	m_scanType->addItems({QStringLiteral("Auto"), QStringLiteral("Top-Field First"), QStringLiteral("Bottom-Field First"), QStringLiteral("Progressive")});
	form->addRow(QStringLiteral("Scan type"), m_scanType);

	m_arMode = new QCheckBox(QStringLiteral("Read AR from stream"));
	m_arMode->setTristate(true);
	form->addRow(m_arMode);

	m_discardMode = new QCheckBox(QStringLiteral("Skip B-Frames"));
	form->addRow(m_discardMode);

	return group;
}

QWidget* MainWindow::BuildHwAccelerationGroup()
{
	auto* group = new QGroupBox(QStringLiteral("Hardware acceleration"));
	auto* layout = new QVBoxLayout(group);

	layout->addWidget(new QLabel(QStringLiteral("Codecs:")));
	for (size_t i = 0; i < HwCodecLabels.size(); i++) {
		auto* checkBox = new QCheckBox(QString::fromWCharArray(HwCodecLabels[i]));
		m_hwCodecs.push_back(checkBox);
		layout->addWidget(checkBox);
	}

	auto* form = new QFormLayout;
	m_hwDecoder = new QComboBox;
	m_hwDecoder->addItems({QStringLiteral("DXVA2"), QStringLiteral("D3D11, DXVA2"), QStringLiteral("D3D11cb"), QStringLiteral("D3D12cb"), QStringLiteral("NVDEC (Nvidia only)")});
	form->addRow(QStringLiteral("Preferred decoder"), m_hwDecoder);

	m_hwAdapter = new QLineEdit;
	form->addRow(QStringLiteral("Adapter (VendorId:DeviceId, hex)"), m_hwAdapter);

	m_dxvaCheck = new QComboBox;
	m_dxvaCheck->addItems({QStringLiteral("Full check"), QStringLiteral("Skip level check"), QStringLiteral("Skip ref frame check"), QStringLiteral("Skip all checks")});
	form->addRow(QStringLiteral("DXVA (H.264) Compatibility check"), m_dxvaCheck);
	layout->addLayout(form);

	m_disableDxvaSd = new QCheckBox(QStringLiteral("Disable DXVA for SD (H.264)"));
	layout->addWidget(m_disableDxvaSd);

	return group;
}

QWidget* MainWindow::BuildOutputFormatsGroup()
{
	auto* group = new QGroupBox(QStringLiteral("Output formats"));
	auto* layout = new QVBoxLayout(group);

	for (size_t i = 0; i < PixelFormatLabels.size(); i++) {
		auto* checkBox = new QCheckBox(QString::fromWCharArray(PixelFormatLabels[i]));
		m_pixelFormats.push_back(checkBox);
		layout->addWidget(checkBox);
	}

	m_swConvertToRgb = new QCheckBox(QStringLiteral("Convert to RGB"));
	layout->addWidget(m_swConvertToRgb);

	auto* form = new QFormLayout;
	m_swRgbLevels = new QComboBox;
	m_swRgbLevels->addItems({QStringLiteral("PC (0-255)"), QStringLiteral("TV (16-235)")});
	form->addRow(QStringLiteral("RGB output levels:"), m_swRgbLevels);
	layout->addLayout(form);

	return group;
}

QWidget* MainWindow::BuildStatusGroup()
{
	auto* group = new QGroupBox(QStringLiteral("Status"));
	auto* stack = new QStackedLayout(group);

	m_statusNotConnected = new QLabel(QStringLiteral("Not connected (no MPCVideoDec instance is currently decoding)"));
	m_statusNotConnected->setWordWrap(true);

	m_statusConnected = new QWidget;
	auto* grid = new QGridLayout(m_statusConnected);
	grid->setContentsMargins(0, 0, 0, 0);
	m_statusInputFormat = new QLabel;
	m_statusFrameSize = new QLabel;
	m_statusOutputFormat = new QLabel;
	m_statusDecoder = new QLabel;
	m_statusGraphicsAdapter = new QLabel;
	const struct { const char* label; QLabel* value; } rows[] = {
		{"Input format:", m_statusInputFormat},
		{"Frame size:", m_statusFrameSize},
		{"Output format:", m_statusOutputFormat},
		{"Decoder:", m_statusDecoder},
		{"Graphics Adapter:", m_statusGraphicsAdapter},
	};
	int row = 0;
	for (const auto& r : rows) {
		grid->addWidget(new QLabel(QString::fromLatin1(r.label)), row, 0);
		grid->addWidget(r.value, row, 1);
		row++;
	}

	stack->addWidget(m_statusNotConnected);
	stack->addWidget(m_statusConnected);
	stack->setCurrentWidget(m_statusNotConnected);

	return group;
}

void MainWindow::Load()
{
	HKEY key = OpenOrCreate();

	m_threadNumber->setCurrentIndex(static_cast<int>(GetDWord(key, ThreadNumber, 0)));
	m_scanType->setCurrentIndex(static_cast<int>(GetDWord(key, ScanType, 0)));
	m_arMode->setCheckState(ArModeToCheckState(GetDWord(key, ARMode, 2)));
	m_discardMode->setChecked(GetDWord(key, DiscardMode, 0) != 0);

	for (size_t i = 0; i < m_hwCodecs.size(); i++) {
		m_hwCodecs[i]->setChecked(GetDWord(key, HwCodecNames[i], 1) != 0);
	}

	m_hwDecoder->setCurrentIndex(static_cast<int>(GetDWord(key, HwDecoder, 1)));
	m_hwAdapter->setText(QString::fromStdWString(GetString(key, HwAdapter, L"0000:0000")));
	m_dxvaCheck->setCurrentIndex(static_cast<int>(GetDWord(key, DXVACheckCompatibility, 1)));
	m_disableDxvaSd->setChecked(GetDWord(key, DisableDXVA_SD, 0) != 0);

	for (size_t i = 0; i < m_pixelFormats.size(); i++) {
		const bool defaultOn = !IsDefaultDisabledPixelFormat(PixelFormatNames[i]);
		const std::wstring valueName = SwName(PixelFormatNames[i]);
		m_pixelFormats[i]->setChecked(GetDWord(key, valueName.c_str(), defaultOn ? 1 : 0) != 0);
	}

	m_swConvertToRgb->setChecked(GetDWord(key, SwConvertToRGB, 0) != 0);
	m_swRgbLevels->setCurrentIndex(static_cast<int>(GetDWord(key, SwRGBLevels, 0)));

	if (key) {
		RegCloseKey(key);
	}
}

void MainWindow::Save()
{
	HKEY key = OpenOrCreate();

	SetDWord(key, ThreadNumber, static_cast<DWORD>(m_threadNumber->currentIndex()));
	SetDWord(key, ScanType, static_cast<DWORD>(m_scanType->currentIndex()));
	SetDWord(key, ARMode, CheckStateToArMode(m_arMode->checkState()));
	SetDWord(key, DiscardMode, m_discardMode->isChecked() ? 1 : 0);

	for (size_t i = 0; i < m_hwCodecs.size(); i++) {
		SetDWord(key, HwCodecNames[i], m_hwCodecs[i]->isChecked() ? 1 : 0);
	}

	SetDWord(key, HwDecoder, static_cast<DWORD>(m_hwDecoder->currentIndex()));
	SetString(key, HwAdapter, m_hwAdapter->text().toStdWString());
	SetDWord(key, DXVACheckCompatibility, static_cast<DWORD>(m_dxvaCheck->currentIndex()));
	SetDWord(key, DisableDXVA_SD, m_disableDxvaSd->isChecked() ? 1 : 0);

	for (size_t i = 0; i < m_pixelFormats.size(); i++) {
		const std::wstring valueName = SwName(PixelFormatNames[i]);
		SetDWord(key, valueName.c_str(), m_pixelFormats[i]->isChecked() ? 1 : 0);
	}

	SetDWord(key, SwConvertToRGB, m_swConvertToRgb->isChecked() ? 1 : 0);
	SetDWord(key, SwRGBLevels, static_cast<DWORD>(m_swRgbLevels->currentIndex()));

	if (key) {
		RegCloseKey(key);
	}
}

// Matches CMPCVideoDecSettingsWnd::OnBnClickedReset() exactly.
void MainWindow::Reset()
{
	m_threadNumber->setCurrentIndex(0);
	m_scanType->setCurrentIndex(0); // SCAN_AUTO
	m_arMode->setCheckState(Qt::PartiallyChecked);
	m_discardMode->setChecked(false);

	for (auto* checkBox : m_hwCodecs) {
		checkBox->setChecked(true);
	}

	m_hwDecoder->setCurrentIndex(1); // HWDec_D3D11 (this app targets Windows 10+, always D3D11-capable)
	m_hwAdapter->setText(QStringLiteral("0000:0000"));
	m_dxvaCheck->setCurrentIndex(1);
	m_disableDxvaSd->setChecked(false);

	for (size_t i = 0; i < m_pixelFormats.size(); i++) {
		m_pixelFormats[i]->setChecked(!IsDefaultDisabledPixelFormat(PixelFormatNames[i]));
	}

	m_swConvertToRgb->setChecked(false);
	m_swRgbLevels->setCurrentIndex(0);
}

void MainWindow::RefreshStatus()
{
	MPCVideoDecStatusBlock block{};
	auto* stack = static_cast<QStackedLayout*>(m_statusConnected->parentWidget()->layout());

	if (!ReadStatusBlock(block)) {
		stack->setCurrentWidget(m_statusNotConnected);
		return;
	}

	m_statusInputFormat->setText(QString::fromWCharArray(block.InputFormat));
	m_statusFrameSize->setText(QString::fromWCharArray(block.FrameSize));
	m_statusOutputFormat->setText(QString::fromWCharArray(block.OutputFormat));
	m_statusDecoder->setText(QString::fromWCharArray(block.ActiveDecoder));
	m_statusGraphicsAdapter->setText(QString::fromWCharArray(block.GraphicsAdapter));
	stack->setCurrentWidget(m_statusConnected);
}
