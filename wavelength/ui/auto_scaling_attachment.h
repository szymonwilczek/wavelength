#ifndef AUTO_SCALING_ATTACHMENT_H
#define AUTO_SCALING_ATTACHMENT_H

#include <QWidget>
#include <QVBoxLayout>
#include <QResizeEvent>
#include <QTimer>

// Klasa opakowująca załączniki do automatycznego skalowania
class AutoScalingAttachment : public QWidget {
    Q_OBJECT
    
public:
    AutoScalingAttachment(QWidget* content, QWidget* parent = nullptr)
        : QWidget(parent), m_originalContent(content)
    {
        // Tworzymy layout
        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        
        // Zachowujemy oryginalny rozmiar
        m_originalSize = content->sizeHint();
        
        // Dodajemy zawartość do layoutu
        layout->addWidget(content, 0, Qt::AlignCenter);
        
        // Ustawiamy odpowiednią politykę rozmiaru
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        
        // Timer do opóźnionego dostosowania rozmiaru
        m_resizeTimer = new QTimer(this);
        m_resizeTimer->setSingleShot(true);
        connect(m_resizeTimer, &QTimer::timeout, this, &AutoScalingAttachment::adjustContentSize);
    }
    
    QSize sizeHint() const override {
        // Zwracamy oryginalny rozmiar jako sugerowany
        return m_originalSize;
    }
    
protected:
    void resizeEvent(QResizeEvent* event) override {
        // Uruchamiamy timer przy zmianie rozmiaru
        m_resizeTimer->start(50);
        QWidget::resizeEvent(event);
    }
    
private slots:
    void adjustContentSize() {
        // Pobieramy aktualny rozmiar widgetu
        QSize currentSize = size();
        
        // Obliczamy współczynnik skalowania
        qreal widthRatio = (qreal)currentSize.width() / m_originalSize.width();
        qreal heightRatio = (qreal)currentSize.height() / m_originalSize.height();
        
        // Wybieramy mniejszy współczynnik, aby zachować proporcje
        qreal ratio = qMin(widthRatio, heightRatio);
        ratio = qMin(ratio, 1.0); // Nie skalujemy większych niż oryginalne
        
        // Obliczamy nowy rozmiar
        QSize newSize(m_originalSize.width() * ratio, m_originalSize.height() * ratio);
        
        // Aktualizujemy rozmiar contentu
        // Tylko jeśli content ma metodę resize() (np. QLabel)
        if (m_originalContent) {
            if (m_originalContent->inherits("QLabel")) {
                m_originalContent->resize(newSize);
            } else if (m_originalContent->inherits("InlineImageViewer")) {
                m_originalContent->resize(newSize);
            } else if (m_originalContent->inherits("InlineGifPlayer")) {
                m_originalContent->resize(newSize);
            } else if (m_originalContent->inherits("InlineAudioPlayer")) {
                // Dla audio playera ustawiamy tylko szerokość
                m_originalContent->setFixedWidth(newSize.width());
            }
            
            // Centrujemy zawartość w dostępnej przestrzeni
            int x = (currentSize.width() - m_originalContent->width()) / 2;
            int y = (currentSize.height() - m_originalContent->height()) / 2;
            m_originalContent->move(x, y);
        }
    }
    
private:
    QWidget* m_originalContent;
    QSize m_originalSize;
    QTimer* m_resizeTimer;
};

#endif // AUTO_SCALING_ATTACHMENT_H