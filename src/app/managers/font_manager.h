#ifndef FONT_MANAGER_H
#define FONT_MANAGER_H

#include <QObject>
#include <QMap>
#include <QString>
#include <QFont>

class FontManager final : public QObject {
    Q_OBJECT

public:
    static FontManager* GetInstance();

    bool Initialize();

    QString GetFontFamily(const QString& font_name);
    
    QFont GetFont(const QString& font_name, int point_size = 10, int weight = QFont::Normal);

private:
    explicit FontManager(QObject* parent = nullptr);
    ~FontManager() override = default;

    static FontManager* instance_;

    QMap<QString, QString> font_families_;

    bool LoadFont(const QString& font_name, const QString& font_path);
};

#endif // FONT_MANAGER_H