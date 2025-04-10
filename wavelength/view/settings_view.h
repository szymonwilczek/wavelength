#ifndef SETTINGS_VIEW_H
#define SETTINGS_VIEW_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>
#include <QComboBox>
#include <QSpinBox>
#include <QStackedWidget>
#include <QScrollArea>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QDateTime>
#include <QPainter>
#include <QProgressBar>

#include "../ui/button/cyber_button.h"
#include "../ui/checkbox/cyber_checkbox.h"
#include "../ui/input/cyber_line_edit.h"
#include "../util/wavelength_config.h"

// Klasa dla przycisków zakładek z efektem podkreślenia
class TabButton : public QPushButton {
    Q_OBJECT
    Q_PROPERTY(double underlineOffset READ underlineOffset WRITE setUnderlineOffset)

public:
    explicit TabButton(const QString &text, QWidget *parent = nullptr)
        : QPushButton(text, parent), m_underlineOffset(0), m_isActive(false) {

        setFlat(true);
        setCursor(Qt::PointingHandCursor);
        setCheckable(true);

        // Styl przycisku zakładki
        setStyleSheet(
            "TabButton {"
            "  color: #00ccff;"
            "  background-color: transparent;"
            "  border: none;"
            "  font-family: 'Consolas';"
            "  font-size: 11pt;"
            "  padding: 5px 15px;"
            "  margin: 0px 10px;"
            "  text-align: center;"
            "}"
            "TabButton:hover {"
            "  color: #00eeff;"
            "}"
            "TabButton:checked {"
            "  color: #ffffff;"
            "}"
        );
    }

    double underlineOffset() const { return m_underlineOffset; }

    void setUnderlineOffset(double offset) {
        m_underlineOffset = offset;
        update();
    }

    void setActive(bool active) {
        m_isActive = active;
        update();
    }

protected:
    void enterEvent(QEvent *event) override {
        if (!m_isActive) {
            QPropertyAnimation *anim = new QPropertyAnimation(this, "underlineOffset");
            anim->setDuration(300);
            anim->setStartValue(0.0);
            anim->setEndValue(5.0);
            anim->setEasingCurve(QEasingCurve::InOutQuad);
            anim->start(QPropertyAnimation::DeleteWhenStopped);
        }
        QPushButton::enterEvent(event);
    }

    void leaveEvent(QEvent *event) override {
        if (!m_isActive) {
            QPropertyAnimation *anim = new QPropertyAnimation(this, "underlineOffset");
            anim->setDuration(300);
            anim->setStartValue(m_underlineOffset);
            anim->setEndValue(0.0);
            anim->setEasingCurve(QEasingCurve::InOutQuad);
            anim->start(QPropertyAnimation::DeleteWhenStopped);
        }
        QPushButton::leaveEvent(event);
    }

    void paintEvent(QPaintEvent *event) override {
        QPushButton::paintEvent(event);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        QColor underlineColor = m_isActive ? QColor(0, 220, 255) : QColor(0, 180, 220, 180);

        // Rysowanie podkreślenia
        int lineY = height() - 5;

        if (m_isActive) {
            // Aktywna zakładka ma pełne podkreślenie
            painter.setPen(QPen(underlineColor, 2));
            painter.drawLine(5, lineY, width() - 5, lineY);
        } else if (m_underlineOffset > 0) {
            // Zakładka z animującym się podkreśleniem przy najechaniu
            painter.setPen(QPen(underlineColor, 1.5));

            // Animowane podkreślenie porusza się lekko w poziomie
            double offset = sin(m_underlineOffset) * 2.0;
            int centerX = width() / 2;
            int lineWidth = width() * 0.6 * (m_underlineOffset / 5.0);

            painter.drawLine(centerX - lineWidth/2 + offset, lineY,
                            centerX + lineWidth/2 + offset, lineY);
        }
    }

private:
    double m_underlineOffset;
    bool m_isActive;
};

class SettingsView : public QWidget {
    Q_OBJECT

public:
    explicit SettingsView(QWidget *parent = nullptr);
    ~SettingsView() override;

signals:
    void backToMainView();
    void settingsChanged();
    // Usuń te sygnały:
    // void enterSettingsView();
    // void leaveSettingsView();

protected:
    void showEvent(QShowEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void saveSettings();
    void restoreDefaults();
    void switchToTab(int tabIndex);
    void processFingerprint(bool completed = false);
    void processHandprint(bool completed = false);
    void checkSecurityCode();
    void securityQuestionTimeout();

private:
    void loadSettingsFromRegistry();
    void setupUi();
    void setupUserTab();
    void setupServerTab();
    void setupAppearanceTab();
    void setupNetworkTab();
    void setupAdvancedTab();
    void setupClassifiedTab();
    void setupNextSecurityLayer();
    void generateRandomFingerprint(QLabel* targetLabel);
    void generateRandomHandprint(QLabel* targetLabel);
    int getRandomSecurityCode(QString& hint);

    enum SecurityLayer {
        Fingerprint = 0,
        Handprint,
        SecurityCode,
        SecurityQuestion,
        AccessGranted
    };

    SecurityLayer m_currentSecurityLayer;
    QStackedWidget* m_securityLayersStack;

    // Widgets dla poszczególnych zabezpieczeń
    QWidget* m_fingerprintWidget;
    QLabel* m_fingerprintImage;
    QProgressBar* m_fingerprintProgress;

    QWidget* m_handprintWidget;
    QLabel* m_handprintImage;
    QProgressBar* m_handprintProgress;

    QWidget* m_securityCodeWidget;
    QLineEdit* m_securityCodeInput;
    QLabel* m_securityCodeHint;
    int m_currentSecurityCode;

    QWidget* m_securityQuestionWidget;
    QLabel* m_securityQuestionLabel;
    QLineEdit* m_securityQuestionInput;
    QTimer* m_securityQuestionTimer;

    QWidget* m_accessGrantedWidget;

    // Timery i animacje
    QTimer* m_fingerprintTimer;
    QTimer* m_handprintTimer;

    void createHeaderPanel();

    WavelengthConfig *m_config;
    QStackedWidget *m_tabContent;
    QLabel *m_titleLabel;
    QLabel *m_sessionLabel;
    QLabel *m_timeLabel;
    QWidget *m_tabBar;
    QList<TabButton*> m_tabButtons;

    // Kontrolki do edycji ustawień - User
    CyberLineEdit *m_userNameEdit;

    // Kontrolki do edycji ustawień - Server
    CyberLineEdit *m_serverAddressEdit;
    QSpinBox *m_serverPortEdit;

    // Kontrolki do edycji ustawień - Appearance
    QComboBox *m_themeComboBox;
    QSpinBox *m_animationDurationEdit;

    // Kontrolki do edycji ustawień - Network
    QSpinBox *m_connectionTimeoutEdit;
    QSpinBox *m_keepAliveIntervalEdit;
    QSpinBox *m_maxReconnectAttemptsEdit;

    // Kontrolki do edycji ustawień - Advanced
    CyberCheckBox *m_debugModeCheckBox;
    QSpinBox *m_chatHistorySizeEdit;
    QSpinBox *m_processedMessageIdsEdit;
    QSpinBox *m_sentMessageCacheSizeEdit;
    QSpinBox *m_maxRecentWavelengthEdit;

    // Przyciski akcji
    CyberButton *m_saveButton;
    CyberButton *m_defaultsButton;
    CyberButton *m_backButton;

    QTimer *m_refreshTimer;
};

#endif // SETTINGS_VIEW_H