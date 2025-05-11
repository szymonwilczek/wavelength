#ifndef NETWORK_STATUS_WIDGET_H
#define NETWORK_STATUS_WIDGET_H

#include <QWidget>

class QNetworkAccessManager;
class QLabel;
class TranslationManager;

/**
 * @brief A widget displaying network connection status and ping time with cyberpunk aesthetics.
 *
 * This widget periodically checks network connectivity by sending a request to a target server
 * (currently Google, intended to be a Wavelength server). It displays the status ("SYSTEM READY",
 * "OFFLINE", etc.), the ping time in milliseconds, and a WiFi-style icon whose color and
 * number of active arcs reflect the connection quality (Excellent, Good, Fair, Poor, None).
 * The widget features a custom-drawn background and border with colors changing based on quality.
 */
class NetworkStatusWidget final : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructs a NetworkStatusWidget.
     * Initializes UI elements (labels, layout), sets appearance (size, transparency, glow effect),
     * creates the network manager, starts the timer for periodic status checks, and performs
     * an initial check.
     * @param parent Optional parent widget.
     */
    explicit NetworkStatusWidget(QWidget *parent = nullptr);

    /**
     * @brief Destructor. Stops the update timer.
     */
    ~NetworkStatusWidget() override;

public slots:
    /**
     * @brief Performs a network status check.
     * Sends a network request, measures the response time (ping), determines the
     * NetworkQuality based on the result, and calls UpdateStatusDisplay().
     * Triggered by the update_timer_ and can be called manually.
     */
    void CheckNetworkStatus();

protected:
    /**
     * @brief Overridden paint event handler. Draws the custom widget background and border.
     * Renders a rounded rectangle background and a border whose color depends on the
     * current network quality (border_color_).
     * @param event The paint event.
     */
    void paintEvent(QPaintEvent *event) override;

private:
    /**
     * @brief Enum representing the perceived quality of the network connection.
     */
    enum NetworkQuality {
        kNone, ///< No connection detected.
        kPoor, ///< Very high latency or unstable connection.
        kFair, ///< Moderate latency.
        kGood, ///< Good latency.
        kExcellent ///< Low latency.
    };

    /**
     * @brief Updates the text of the status and ping labels, the icon, and the border color
     * based on the current_quality_ and ping_value_. Triggers a repaint.
     */
    void UpdateStatusDisplay();

    /**
     * @brief Generates the WiFi-style icon as a QPixmap based on the network quality.
     * Draws a series of arcs, coloring them based on the quality level. Sets the generated
     * pixmap on the icon_label_.
     * @param quality The current network quality.
     */
    void CreateNetworkIcon(NetworkQuality quality) const;

    /**
     * @brief Static utility function to get the appropriate color for a given network quality level.
     * Used for the border, text, and icon color.
     * @param quality The network quality level.
     * @return The corresponding QColor.
     */
    static QColor GetQualityColor(NetworkQuality quality);

    /** @brief Label displaying the textual status (e.g., "SYSTEM READY"). */
    QLabel *status_label_;
    /** @brief Label displaying the measured ping time in milliseconds. */
    QLabel *ping_label_;
    /** @brief Label displaying the WiFi-style icon. */
    QLabel *icon_label_;
    /** @brief Manages network requests for status checking. */
    QNetworkAccessManager *network_manager_;
    /** @brief Timer triggering periodic calls to CheckNetworkStatus(). */
    QTimer *update_timer_;
    /** @brief The currently determined network quality level. */
    NetworkQuality current_quality_;
    /** @brief The color used for the border, text, and active icon parts, based on current_quality_. */
    QColor border_color_;
    /** @brief The last measured ping time in milliseconds. */
    qint64 ping_value_;
    /** @brief Pointer to the translation manager for handling UI translations. */
    TranslationManager *translator_ = nullptr;
};

#endif // NETWORK_STATUS_WIDGET_H
