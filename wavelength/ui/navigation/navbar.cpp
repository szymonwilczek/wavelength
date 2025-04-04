#include "navbar.h"
#include "../button/cyberpunk_button.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QSpacerItem>
#include <QFont>
#include <QContextMenuEvent>
#include <QPushButton>
#include <QPainter>
#include <QDateTime>
#include <QRandomGenerator>
#include <QtMath>
#include <QGraphicsBlurEffect>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsView>
#include <QEvent>
#include <QMouseEvent>

// Struktura reprezentująca punkt danych dla animacji przepływu
struct DataPoint {
    QPointF position;      // Pozycja punktu
    QPointF velocity;      // Prędkość i kierunek ruchu
    QColor color;          // Kolor punktu
    double size;           // Rozmiar punktu
    double lifetime;       // Pozostały czas życia (w sekundach)
    double maxLifetime;    // Całkowity czas życia
};

// Klasa renderująca navbar z efektem głębi 3D i animacjami
class DepthNavBackground : public QWidget {
public:
    DepthNavBackground(QWidget *parent = nullptr) : QWidget(parent) {
        // Kolory dopasowane do animacji bloba
        m_gridColor = QColor(0, 195, 255, 25);      // Neonowy niebieski (zmniejszona intensywność)
        m_accentColor = QColor(0, 130, 200, 20);    // Jaśniejszy niebieski (zmniejszona intensywność)
        m_hlColor = QColor(13, 220, 255, 60);       // Jasny niebieski (zmniejszona intensywność)
        m_purpleAccent = QColor(220, 0, 255, 35);   // Cyberpunkowa purpura (zmniejszona intensywność)
        m_greenAccent = QColor(0, 255, 180, 35);    // Neonowa zieleń (zmniejszona intensywność)

        // Inicjalizacja czasomierzy
        m_lastUpdateTime = QDateTime::currentMSecsSinceEpoch();

        // Timer do odświeżania animacji
        m_animTimer = new QTimer(this);
        connect(m_animTimer, &QTimer::timeout, this, QOverload<>::of(&DepthNavBackground::update));
        m_animTimer->start(16); // ~33fps

        // Styl przeźroczystego tła
        setAttribute(Qt::WA_TranslucentBackground);
        setAutoFillBackground(false);

        // Inicjalizacja linii skanowania
        m_scanLine1Position = -50;
        m_scanLine2Position = -200;

        // Inicjalizacja warstw renderingu
        initLayers();

        // Generowanie początkowych punktów danych
        generateDataPoints();
    }

    ~DepthNavBackground() {
        // Zwolnienie zasobów
        for (auto buffer : m_layerBuffers) {
            delete buffer;
        }
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

        // Obliczenie upływu czasu
        qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
        double deltaTime = (currentTime - m_lastUpdateTime) / 1000.0;
        m_lastUpdateTime = currentTime;

        // Aktualizacja fazy animacji
        m_animPhase += deltaTime * 0.2; // Wolniejsza animacja
        if (m_animPhase > 1.0) m_animPhase -= 1.0;

        // Renderowanie warstw od najgłębszej do najbliższej
        drawBaseBackground(painter); // Ciemne tło

        // Rysowanie warstw statycznych
        for (int i = 0; i < m_layerBuffers.size(); i++) {
            if (!m_layerBuffers[i] || m_layerBuffers[i]->isNull()) continue;
            painter.drawPixmap(0, 0, *m_layerBuffers[i]);
        }

        // Rysowanie dynamicznych elementów
        drawScanLines(painter, deltaTime);
        updateAndDrawDataPoints(painter, deltaTime);
    }

    void resizeEvent(QResizeEvent *event) override {
        QWidget::resizeEvent(event);

        // Regeneracja wszystkich warstw po zmianie rozmiaru
        initLayers();
        generateDataPoints();
    }

private:
    void initLayers() {
        // Zwalnianie poprzednich buforów jeśli istnieją
        for (auto buffer : m_layerBuffers) {
            delete buffer;
        }
        m_layerBuffers.clear();

        if (width() <= 0 || height() <= 0) return;

        // Warstwa 1 - Najgłębsza - subtelna siatka
        QPixmap *deepGridLayer = new QPixmap(width(), height());
        deepGridLayer->fill(Qt::transparent);
        QPainter deepGridPainter(deepGridLayer);
        deepGridPainter.setRenderHint(QPainter::Antialiasing, true);
        drawDeepGrid(deepGridPainter, width(), height());
        m_layerBuffers.push_back(deepGridLayer);

        // Warstwa 2 - Płytka głębokość - główna siatka
        QPixmap *mainGridLayer = new QPixmap(width(), height());
        mainGridLayer->fill(Qt::transparent);
        QPainter mainGridPainter(mainGridLayer);
        mainGridPainter.setRenderHint(QPainter::Antialiasing, true);
        drawMainGrid(mainGridPainter, width(), height());
        m_layerBuffers.push_back(mainGridLayer);

        // Warstwa 3 - Najbliższa - akcenty narożne i elementy dekoracyjne
        QPixmap *accentsLayer = new QPixmap(width(), height());
        accentsLayer->fill(Qt::transparent);
        QPainter accentsPainter(accentsLayer);
        accentsPainter.setRenderHint(QPainter::Antialiasing, true);
        drawAccents(accentsPainter, width(), height());
        m_layerBuffers.push_back(accentsLayer);
    }

    void drawBaseBackground(QPainter &painter) {
        // Ciemne tło z subtelnym gradientem
        QLinearGradient bgGradient(0, 0, width(), height());
        bgGradient.setColorAt(0, QColor(0, 15, 30, 190)); // Mniejsza nieprzezroczystość
        bgGradient.setColorAt(1, QColor(5, 25, 40, 210));
        painter.fillRect(rect(), bgGradient);
    }

    void drawDeepGrid(QPainter &painter, int width, int height) {
        // Najgłębsza warstwa - bardzo subtelna siatka
        QColor deepGridColor = m_gridColor;
        deepGridColor.setAlphaF(0.15); // Bardzo subtelny

        painter.setPen(QPen(deepGridColor, 0.5));

        // Duże kwadraty siatki
        int gridSpacing = 40;
        for (int y = 0; y < height; y += gridSpacing) {
            painter.drawLine(0, y, width, y);
        }

        for (int x = 0; x < width; x += gridSpacing) {
            painter.drawLine(x, 0, x, height);
        }

        // Dodajemy kilka przekątnych linii dla efektu głębi
        painter.setPen(QPen(deepGridColor, 0.5, Qt::DotLine));
        painter.drawLine(0, 0, width, height);
        painter.drawLine(width, 0, 0, height);
        painter.drawLine(width/2, 0, width/2, height);
        painter.drawLine(0, height/2, width, height/2);
    }

    void generateDataPoints() {
        // Wyczyść istniejące punkty
        m_dataPoints.clear();

        if (width() <= 0 || height() <= 0) return;

        // Generowanie nowych punktów danych
        int numPoints = 10 + (width() * height() / 15000); // Ilość punktów zależna od rozmiaru
        QRandomGenerator *rng = QRandomGenerator::global();

        for (int i = 0; i < numPoints; i++) {
            DataPoint point;

            // Losowa pozycja początkowa
            point.position.setX(rng->bounded(width()));
            point.position.setY(rng->bounded(height()));

            // Losowa prędkość (powolny ruch)
            double angle = rng->bounded(2.0 * M_PI);
            double speed = 5 + rng->bounded(15.0);
            point.velocity.setX(speed * cos(angle));
            point.velocity.setY(speed * sin(angle));

            // Losowy kolor
            int colorChoice = rng->bounded(5);
            switch(colorChoice) {
                case 0: point.color = m_hlColor; break;
                case 1: point.color = m_purpleAccent; break;
                case 2: point.color = m_greenAccent; break;
                default: point.color = m_accentColor; break;
            }

            // Zwiększ nieprzezroczystość dla lepszej widoczności
            point.color.setAlphaF(point.color.alphaF() * 1.5);

            // Pozostałe właściwości
            point.size = 1.0 + rng->bounded(2.0);
            point.maxLifetime = 4.0 + rng->bounded(4.0); // 4-8 sekund życia
            point.lifetime = point.maxLifetime;

            m_dataPoints.push_back(point);
        }
    }

    void updateAndDrawDataPoints(QPainter &painter, double deltaTime) {
        QVector<DataPoint> updatedPoints;

        for (DataPoint &point : m_dataPoints) {
            // Aktualizacja czasu życia
            point.lifetime -= deltaTime;

            if (point.lifetime <= 0) {
                // Generowanie nowego punktu zamiast umierającego
                generateNewDataPoint(updatedPoints);
                continue;
            }

            // Aktualizacja pozycji
            point.position += point.velocity * deltaTime;

            // Sprawdzenie czy punkt wyszedł poza granice
            if (point.position.x() < -10 || point.position.x() > width() + 10 ||
                point.position.y() < -10 || point.position.y() > height() + 10) {
                // Generowanie nowego punktu zamiast wychodzącego poza ekran
                generateNewDataPoint(updatedPoints);
                continue;
            }

            // Rysowanie punktu
            double fadeMultiplier = point.lifetime / point.maxLifetime; // Wygaszanie pod koniec życia
            QColor pointColor = point.color;
            pointColor.setAlphaF(pointColor.alphaF() * fadeMultiplier);

            painter.setPen(Qt::NoPen);
            painter.setBrush(pointColor);
            painter.drawEllipse(point.position, point.size, point.size);

            // Dodaj subtelną poświatę
            QRadialGradient gradient(point.position, point.size * 3);
            QColor glowColor = pointColor;
            glowColor.setAlphaF(glowColor.alphaF() * 0.3);
            gradient.setColorAt(0, glowColor);
            gradient.setColorAt(1, QColor(0, 0, 0, 0));

            painter.setBrush(gradient);
            painter.drawEllipse(point.position, point.size * 3, point.size * 3);

            // Zachowanie punktu
            updatedPoints.push_back(point);
        }

        // Aktualizacja listy punktów
        m_dataPoints = updatedPoints;

        // Losowe dodawanie nowych punktów
        if (m_dataPoints.size() < 15 || QRandomGenerator::global()->bounded(100) < 2) {
            generateNewDataPoint(m_dataPoints);
        }

        // Rysowanie połączeń między pobliskimi punktami
        drawDataConnections(painter);
    }

    void generateNewDataPoint(QVector<DataPoint> &points) {
        if (width() <= 10 || height() <= 10) return;

        QRandomGenerator *rng = QRandomGenerator::global();
        DataPoint point;

        // Generowanie nowego punktu na brzegu ekranu
        int edge = rng->bounded(4); // 0:góra, 1:prawo, 2:dół, 3:lewo

        switch(edge) {
            case 0: // Góra
                point.position.setX(rng->bounded(width()));
                point.position.setY(-5);
                break;
            case 1: // Prawo
                point.position.setX(width() + 5);
                point.position.setY(rng->bounded(height()));
                break;
            case 2: // Dół
                point.position.setX(rng->bounded(width()));
                point.position.setY(height() + 5);
                break;
            case 3: // Lewo
                point.position.setX(-5);
                point.position.setY(rng->bounded(height()));
                break;
        }

        // Kierunek ruchu w stronę środka ekranu
        QPointF center(width() / 2, height() / 2);
        QPointF direction = center - point.position;
        double length = sqrt(direction.x() * direction.x() + direction.y() * direction.y());
        if (length > 0) {
            direction = direction / length; // Normalizacja wektora
        }

        double speed = 10 + rng->bounded(20.0);
        point.velocity = direction * speed;

        // Losowy kolor
        int colorChoice = rng->bounded(5);
        switch(colorChoice) {
            case 0: point.color = m_hlColor; break;
            case 1: point.color = m_purpleAccent; break;
            case 2: point.color = m_greenAccent; break;
            default: point.color = m_accentColor; break;
        }

        // Zwiększ nieprzezroczystość dla lepszej widoczności
        point.color.setAlphaF(point.color.alphaF() * 1.5);

        point.size = 1.0 + rng->bounded(2.0);
        point.maxLifetime = 4.0 + rng->bounded(4.0); // 4-8 sekund życia
        point.lifetime = point.maxLifetime;

        points.push_back(point);
    }

    void drawDataConnections(QPainter &painter) {
        // Łączenie punktów, które są blisko siebie
        const double MAX_CONNECT_DIST = 100.0;

        for (int i = 0; i < m_dataPoints.size(); i++) {
            for (int j = i + 1; j < m_dataPoints.size(); j++) {
                double dist = QLineF(m_dataPoints[i].position, m_dataPoints[j].position).length();

                if (dist < MAX_CONNECT_DIST) {
                    // Obliczenie przeźroczystości linii (dalsze = bardziej przezroczyste)
                    double alpha = 1.0 - (dist / MAX_CONNECT_DIST);
                    alpha *= 0.2; // Ogólne zmniejszenie nieprzezroczystości

                    // Mieszanie kolorów punktów
                    QColor lineColor = m_dataPoints[i].color;
                    lineColor.setAlphaF(alpha);

                    // Grubość linii (bliższe = grubsze)
                    double penWidth = 0.5 * (1.0 - dist / MAX_CONNECT_DIST);

                    painter.setPen(QPen(lineColor, penWidth, Qt::SolidLine));
                    painter.drawLine(m_dataPoints[i].position, m_dataPoints[j].position);
                }
            }
        }
    }

    void drawScanLines(QPainter &painter, double deltaTime) {
        // Aktualizacja pozycji linii skanowania
        m_scanLine1Position += 20 * deltaTime;
        m_scanLine2Position += 35 * deltaTime;

        if (m_scanLine1Position > height() + 50) m_scanLine1Position = -50;
        if (m_scanLine2Position > height() + 50) m_scanLine2Position = -50;

        // Rysowanie linii skanowania
        QColor scanColor1 = m_hlColor;
        scanColor1.setAlphaF(0.15);
        painter.setPen(QPen(scanColor1, 1.5));
        painter.drawLine(0, m_scanLine1Position, width(), m_scanLine1Position);

        QColor scanColor2 = m_purpleAccent;
        scanColor2.setAlphaF(0.1);
        painter.setPen(QPen(scanColor2, 1));
        painter.drawLine(0, m_scanLine2Position, width(), m_scanLine2Position);

        // Dodanie subtelnego "zaburzenia" przy linii skanowania
        QColor glowColor = scanColor1.lighter(120);
        glowColor.setAlphaF(0.08);

        QLinearGradient grad1(0, m_scanLine1Position - 3, 0, m_scanLine1Position + 3);
        grad1.setColorAt(0, QColor(0, 0, 0, 0));
        grad1.setColorAt(0.5, glowColor);
        grad1.setColorAt(1, QColor(0, 0, 0, 0));

        painter.setPen(Qt::NoPen);
        painter.setBrush(grad1);
        painter.drawRect(0, m_scanLine1Position - 3, width(), 6);
    }

    void drawMainGrid(QPainter &painter, int width, int height) {
        // Główna siatka - bardziej wyraźna niż głęboka siatka
        painter.setPen(QPen(m_gridColor, 0.8));

        int gridSpacing = 20;
        for (int y = 0; y < height; y += gridSpacing) {
            painter.drawLine(0, y, width, y);
        }

        for (int x = 0; x < width; x += gridSpacing) {
            painter.drawLine(x, 0, x, height);
        }

        // Subtelne obszary siatki
        QColor areaColor = m_accentColor;
        areaColor.setAlphaF(0.05);
        painter.setBrush(areaColor);
        painter.setPen(Qt::NoPen);

        // Rysujemy kilka prostokątnych obszarów
        QRandomGenerator *rng = QRandomGenerator::global();
        int numAreas = 3;

        for (int i = 0; i < numAreas; i++) {
            // Sprawdzenie, czy wymiary są wystarczające dla bezpiecznego losowania
            if (width <= 100 || height <= 60) continue;

            int x = rng->bounded(width - 100);
            int y = rng->bounded(height - 60);
            int w = 60 + rng->bounded(1, 81); // Bezpieczne losowanie z zakresu 1-80
            int h = 30 + rng->bounded(1, 41); // Bezpieczne losowanie z zakresu 1-40

            painter.drawRect(x, y, w, h);
        }
    }

    void drawAccents(QPainter &painter, int width, int height) {
        // Najbliższa warstwa - akcenty narożne i dekoracyjne elementy
        int cornerSize = 15;

        // Narożniki z subtelną poświatą
        drawCornerAccent(painter, 0, 0, cornerSize, cornerSize, m_purpleAccent, 0);
        drawCornerAccent(painter, width - cornerSize, 0, cornerSize, cornerSize, m_greenAccent, 1);
        drawCornerAccent(painter, width - cornerSize, height - cornerSize, cornerSize, cornerSize, m_hlColor, 2);
        drawCornerAccent(painter, 0, height - cornerSize, cornerSize, cornerSize, m_accentColor, 3);

        // Subtelna linia górna - bez pulsującej linii dolnej
        painter.setPen(QPen(m_accentColor, 1));
        painter.drawLine(width * 0.05, 1, width * 0.95, 1);

        // Zamiast pulsującej linii dolnej - geometryczny element dekoracyjny
        int bottomDecorHeight = 2;
        QLinearGradient bottomGrad(0, height - bottomDecorHeight, width, height - bottomDecorHeight);
        bottomGrad.setColorAt(0, QColor(0, 0, 0, 0));
        bottomGrad.setColorAt(0.1, m_purpleAccent);
        bottomGrad.setColorAt(0.3, m_hlColor);
        bottomGrad.setColorAt(0.7, m_hlColor);
        bottomGrad.setColorAt(0.9, m_greenAccent);
        bottomGrad.setColorAt(1.0, QColor(0, 0, 0, 0));

        painter.fillRect(0, height - bottomDecorHeight, width, bottomDecorHeight, bottomGrad);
    }

    void drawCornerAccent(QPainter &painter, int x, int y, int width, int height, const QColor &color, int corner) {
        // Rysowanie narożnika z efektem technologicznym
        QColor accentColor = color;
        accentColor.setAlphaF(accentColor.alphaF() * 0.8);

        painter.setPen(QPen(accentColor, 1.5));

        switch(corner) {
            case 0: // Lewy górny
                painter.drawLine(x, y + height, x + width, y);
                painter.drawLine(x, y + height/2, x + width/2, y);
                break;

            case 1: // Prawy górny
                painter.drawLine(x, y, x + width, y + height);
                painter.drawLine(x + width/2, y, x + width, y + height/2);
                break;

            case 2: // Prawy dolny
                painter.drawLine(x, y + height, x + width, y);
                painter.drawLine(x + width/2, y + height, x + width, y + height/2);
                break;

            case 3: // Lewy dolny
                painter.drawLine(x + width, y + height, x, y);
                painter.drawLine(x, y + height/2, x + width/2, y + height);
                break;
        }

        // Dodajemy punkt technologiczny w rogu
        QRadialGradient cornerGlow(x + width/2, y + height/2, width/4);
        accentColor.setAlphaF(accentColor.alphaF() * 0.3);
        cornerGlow.setColorAt(0, accentColor);
        cornerGlow.setColorAt(1, QColor(0, 0, 0, 0));

        painter.setBrush(cornerGlow);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(QPointF(x + width/2, y + height/2), width/4, width/4);
    }

    QColor m_gridColor;
    QColor m_accentColor;
    QColor m_hlColor;
    QColor m_purpleAccent;
    QColor m_greenAccent;

    qint64 m_lastUpdateTime;
    double m_animPhase = 0.0;
    QTimer *m_animTimer;

    // Warstwy z różną głębokością
    QVector<QPixmap*> m_layerBuffers;

    // Linie skanowania
    double m_scanLine1Position = 0.0;
    double m_scanLine2Position = 0.0;

    // Punkty danych dla animacji przepływu
    QVector<DataPoint> m_dataPoints;
};

// Reszta implementacji Navbar pozostaje bez zmian
Navbar::Navbar(QWidget *parent) : QToolBar(parent) {
    setMovable(false);
    setFloatable(false);
    setAllowedAreas(Qt::TopToolBarArea);
    setContextMenuPolicy(Qt::PreventContextMenu);

    // Ustawienie przezroczystego tła dla paska
    setStyleSheet(
        "QToolBar {"
        "  border: none;"
        "  padding: 15px 10px;"
        "  spacing: 15px;"
        "  background-color: transparent;"
        "}"
    );

    // Utworzenie tła z efektem głębi 3D
    DepthNavBackground *navBackground = new DepthNavBackground(this);
    navBackground->setObjectName("navBackground");
    navBackground->lower(); // Umieszczenie tła pod innymi elementami

    // Logo z subtelniejszą, mniej narzucającą się stylistyką
    logoLabel = new QLabel("WAVELENGTH", this);
    QFont logoFont("Arial", 14);  // Zmniejszona wielkość czcionki
    logoFont.setStyleStrategy(QFont::PreferAntialias);
    logoFont.setLetterSpacing(QFont::AbsoluteSpacing, 1);
    logoLabel->setFont(logoFont);
    logoLabel->setStyleSheet(
        "QLabel {"
        "  color: rgba(0, 216, 255, 180);" // Zmniejszona intensywność koloru
        "  font-weight: normal;"  // Usunięte pogrubienie
        "  padding: 5px 15px 5px 15px;"
        "  border-left: 1px solid rgba(0, 200, 255, 40);" // Cieńsza i mniej widoczna ramka
        "  border-right: 1px solid rgba(0, 200, 255, 40);"
        "  background-color: rgba(0, 30, 60, 40);" // Bardziej przezroczyste tło
        "}"
    );

    // Kontener na przyciski z efektem głębi
    QWidget* buttonsContainer = new QWidget(this);
    buttonsContainer->setStyleSheet(
        "background-color: rgba(0, 25, 50, 30);" // Subtelne tło dla przycisków
        "border-radius: 3px;"
    );
    QHBoxLayout* buttonsLayout = new QHBoxLayout(buttonsContainer);
    buttonsLayout->setContentsMargins(15, 8, 15, 8); // Zmniejszone marginesy
    buttonsLayout->setSpacing(30); // Mniejszy odstęp między przyciskami

    // Przyciski
    createWavelengthButton = new CyberpunkButton("Generate Wavelength", buttonsContainer);
    joinWavelengthButton = new CyberpunkButton("Merge Wavelength", buttonsContainer);

    // Dodanie przycisków do layoutu
    buttonsLayout->addWidget(createWavelengthButton, 0, Qt::AlignLeft);
    buttonsLayout->addStretch(); // Elastyczna przestrzeń pomiędzy przyciskami
    buttonsLayout->addWidget(joinWavelengthButton, 0, Qt::AlignRight);

    // Układ: logo po lewej, przyciski z prawej dla lepszej równowagi wizualnej
    QWidget* logoContainer = new QWidget(this);
    QHBoxLayout* logoLayout = new QHBoxLayout(logoContainer);
    logoLayout->setContentsMargins(10, 0, 0, 0);
    logoLayout->addWidget(logoLabel, 0, Qt::AlignLeft);

    // Elastyczna przestrzeń pomiędzy logo a przyciskami
    QWidget* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    // Dodanie elementów do paska narzędzi
    addWidget(logoContainer);
    addWidget(spacer);
    addWidget(buttonsContainer);

    connect(createWavelengthButton, &QPushButton::clicked, this, &Navbar::createWavelengthClicked);
    connect(joinWavelengthButton, &QPushButton::clicked, this, &Navbar::joinWavelengthClicked);
}

void Navbar::contextMenuEvent(QContextMenuEvent *event) {
    event->ignore();
}

void Navbar::resizeEvent(QResizeEvent *event) {
    QToolBar::resizeEvent(event);
    QWidget *background = findChild<DepthNavBackground*>("navBackground");
    if (background) {
        background->setGeometry(rect());
    }
}