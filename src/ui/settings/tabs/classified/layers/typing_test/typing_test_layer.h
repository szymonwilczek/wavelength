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

    void Initialize() override;
    void Reset() override;

    private slots:
        void OnTextChanged(const QString& text);
        void UpdateHighlight() const;

private:
    void GenerateWords();
    void UpdateDisplayText() const;

    QLabel* title_label_;
    QLabel* instructions_label_;
    QLabel* display_text_label_;
    QLineEdit* hidden_input_;

    QStringList words_;
    QString full_text_;
    int current_position_;
    bool test_started_;
    bool test_completed_;
};

#endif // TYPING_TEST_LAYER_H