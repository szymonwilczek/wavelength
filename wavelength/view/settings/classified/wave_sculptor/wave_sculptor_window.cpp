#include "wave_sculptor_window.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QGroupBox>
#include <QCloseEvent>
#include <QDebug>

WaveSculptorWindow::WaveSculptorWindow(QWidget* parent)
    : QWidget(parent, Qt::Window) // Ustaw flagę Qt::Window, aby otworzyć jako osobne okno
{
    setWindowTitle("Wave Sculptor");
    setMinimumSize(600, 300);
    setStyleSheet("background-color: #182028; color: #E0E0E0;"); // Ciemny motyw

    setupUi();

    // Połączenia sygnałów i slotów
    connect(m_glitchButton, &QPushButton::clicked, this, &WaveSculptorWindow::onGlitchButtonClicked);
    connect(m_receiveButton, &QPushButton::clicked, this, &WaveSculptorWindow::onReceiveButtonClicked);
    connect(m_resetButton, &QPushButton::clicked, this, &WaveSculptorWindow::onResetButtonClicked);
    connect(m_glitchSlider, &QSlider::valueChanged, this, &WaveSculptorWindow::onGlitchIntensityChanged);
}

WaveSculptorWindow::~WaveSculptorWindow() {
    qDebug() << "WaveSculptorWindow destroyed";
}

void WaveSculptorWindow::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    // Widget strumienia
    m_streamWidget = new EditableStreamWidget(this);
    mainLayout->addWidget(m_streamWidget, 1); // Rozciągnij widget strumienia

    // Panel kontrolny
    QGroupBox* controlPanel = new QGroupBox("Controls", this);
    controlPanel->setStyleSheet("QGroupBox { border: 1px solid #0077AA; margin-top: 1ex; } "
                                "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 3px 0 3px; }");
    QHBoxLayout* controlLayout = new QHBoxLayout(controlPanel);
    controlLayout->setSpacing(15);

    // Przyciski symulacji
    m_glitchButton = new QPushButton("Trigger Glitch", controlPanel);
    m_receiveButton = new QPushButton("Trigger Receive", controlPanel);
    m_resetButton = new QPushButton("Reset Wave", controlPanel);

    // Suwak intensywności glitcha
    QVBoxLayout* glitchLayout = new QVBoxLayout();
    m_glitchLabel = new QLabel("Glitch Intensity: 0.5", controlPanel);
    m_glitchSlider = new QSlider(Qt::Horizontal, controlPanel);
    m_glitchSlider->setRange(0, 100); // 0 do 1.0 z krokiem 0.01
    m_glitchSlider->setValue(50); // Domyślna wartość 0.5
    m_glitchSlider->setTickInterval(10);
    m_glitchSlider->setTickPosition(QSlider::TicksBelow);
    glitchLayout->addWidget(m_glitchLabel, 0, Qt::AlignCenter);
    glitchLayout->addWidget(m_glitchSlider);

    // Dodawanie kontrolek do layoutu panelu
    controlLayout->addWidget(m_glitchButton);
    controlLayout->addLayout(glitchLayout);
    controlLayout->addWidget(m_receiveButton);
    controlLayout->addWidget(m_resetButton);

    mainLayout->addWidget(controlPanel);

    // Style przycisków
    QString buttonStyle = "QPushButton { background-color: #005577; border: 1px solid #0077AA; padding: 8px; border-radius: 4px; }"
                          "QPushButton:hover { background-color: #0077AA; }"
                          "QPushButton:pressed { background-color: #003355; }";
    m_glitchButton->setStyleSheet(buttonStyle);
    m_receiveButton->setStyleSheet(buttonStyle);
    m_resetButton->setStyleSheet(buttonStyle);
}

void WaveSculptorWindow::onGlitchButtonClicked() {
    qreal intensity = static_cast<qreal>(m_glitchSlider->value()) / 100.0;
    m_streamWidget->triggerGlitch(intensity);
}

void WaveSculptorWindow::onReceiveButtonClicked() {
    m_streamWidget->triggerReceivingAnimation();
}

void WaveSculptorWindow::onResetButtonClicked() {
    // Resetuje punkty fali do linii prostej i animację do idle
    // m_streamWidget->initializeWavePoints(); // Resetuje punkty
    // m_streamWidget->updateVertexBuffer();   // Aktualizuje bufor
    m_streamWidget->returnToIdleAnimation(); // Resetuje animację
    m_streamWidget->update();               // Wymusza odświeżenie
}

void WaveSculptorWindow::onGlitchIntensityChanged(int value) {
    qreal intensity = static_cast<qreal>(value) / 100.0;
    m_glitchLabel->setText(QString("Glitch Intensity: %1").arg(intensity, 0, 'f', 2));
    // Można by też od razu ustawiać intensywność w streamWidget, jeśli chcemy ciągły efekt
    // m_streamWidget->setGlitchIntensity(intensity);
}

void WaveSculptorWindow::closeEvent(QCloseEvent* event) {
    qDebug() << "Closing Wave Sculptor window.";
    // Można dodać logikę zapisu stanu przed zamknięciem, jeśli potrzeba
    QWidget::closeEvent(event);
    // Ważne: Usuń okno po zamknięciu, aby zwolnić pamięć
    deleteLater();
}
