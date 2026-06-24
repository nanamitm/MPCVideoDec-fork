#pragma once

#include <QWidget>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QTimer>
#include <vector>

class MainWindow : public QWidget
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget* parent = nullptr);

private:
	QWidget* BuildVideoSettingsGroup();
	QWidget* BuildHwAccelerationGroup();
	QWidget* BuildOutputFormatsGroup();
	QWidget* BuildStatusGroup();

	void Load();
	void Save();
	void Reset();
	void RefreshStatus();

	QComboBox* m_threadNumber = nullptr;
	QComboBox* m_scanType = nullptr;
	QCheckBox* m_arMode = nullptr;
	QCheckBox* m_discardMode = nullptr;

	std::vector<QCheckBox*> m_hwCodecs;
	QComboBox* m_hwDecoder = nullptr;
	QLineEdit* m_hwAdapter = nullptr;
	QComboBox* m_dxvaCheck = nullptr;
	QCheckBox* m_disableDxvaSd = nullptr;

	std::vector<QCheckBox*> m_pixelFormats;
	QCheckBox* m_swConvertToRgb = nullptr;
	QComboBox* m_swRgbLevels = nullptr;

	QWidget* m_statusConnected = nullptr;
	QLabel* m_statusNotConnected = nullptr;
	QLabel* m_statusInputFormat = nullptr;
	QLabel* m_statusFrameSize = nullptr;
	QLabel* m_statusOutputFormat = nullptr;
	QLabel* m_statusDecoder = nullptr;
	QLabel* m_statusGraphicsAdapter = nullptr;

	QTimer m_statusTimer;
};
