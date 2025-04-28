#ifndef FONT_MANAGER_H
#define FONT_MANAGER_H

#include <QObject>
#include <QMap>
#include <QString>
#include <QFont>

class FontManager final : public QObject {
    Q_OBJECT

public:
    static FontManager* getInstance();

    bool initialize();

    QString getFontFamily(const QString& fontName);
    
    QFont getFont(const QString& fontName, int pointSize = 10, int weight = QFont::Normal);

private:
    explicit FontManager(QObject* parent = nullptr);
    ~FontManager() override = default;

    static FontManager* instance;

    QMap<QString, QString> fontFamilies;

    bool loadFont(const QString& fontName, const QString& fontPath);
};

#endif // FONT_MANAGER_H