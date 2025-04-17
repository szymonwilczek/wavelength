#ifndef WAVE_SCULPTOR_WINDOW_H
#define WAVE_SCULPTOR_WINDOW_H

#include <QWidget>
#include "editable_stream_widget.h"

class QPushButton; // Forward declaration
class QSlider;     // Forward declaration
class QLabel;      // Forward declaration

class WaveSculptorWindow : public QWidget {
    Q_OBJECT

public:
    explicit WaveSculptorWindow(QWidget* parent = nullptr);
    ~WaveSculptorWindow() override;

protected:
    void closeEvent(QCloseEvent* event) override;

    private slots:
        void onGlitchButtonClicked();
    void onReceiveButtonClicked();
    void onResetButtonClicked();
    void onGlitchIntensityChanged(int value);

private:
    void setupUi();

    EditableStreamWidget* m_streamWidget;
    QPushButton* m_glitchButton;
    QPushButton* m_receiveButton;
    QPushButton* m_resetButton;
    QSlider* m_glitchSlider;
    QLabel* m_glitchLabel;
};

#endif // WAVE_SCULPTOR_WINDOW_H