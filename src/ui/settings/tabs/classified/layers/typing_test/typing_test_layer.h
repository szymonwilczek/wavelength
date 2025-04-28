#ifndef TYPING_TEST_LAYER_H
#define TYPING_TEST_LAYER_H

#include "../security_layer.h"
#include <QLabel>
#include <QLineEdit>
#include <QTimer>
#include <QStringList>

class TypingTestLayer final : public SecurityLayer {
    Q_OBJECT

public:
    explicit TypingTestLayer(QWidget *parent = nullptr);
    ~TypingTestLayer() override;

    void initialize() override;
    void reset() override;

    private slots:
        void onTextChanged(const QString& text);
        void updateHighlight() const;

private:
    void generateWords();
    void updateDisplayText() const;

    QLabel* m_titleLabel;
    QLabel* m_instructionsLabel;
    QLabel* m_displayTextLabel;
    QLineEdit* m_hiddenInput;

    QStringList m_words;
    QString m_fullText;
    int m_currentPosition;
    bool m_testStarted;
    bool m_testCompleted;
};

#endif // TYPING_TEST_LAYER_H