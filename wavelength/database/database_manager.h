#ifndef DATABASE_MANAGER_H
#define DATABASE_MANAGER_H

#include <QObject>
#include <QString>
#include <QDebug>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/basic/document.hpp>

class DatabaseManager : public QObject {
    Q_OBJECT

public:
    static DatabaseManager* getInstance() {
        static DatabaseManager instance;
        return &instance;
    }

    bool isFrequencyAvailable(double frequency) {
        if (frequency < 130.0 || frequency > 180000000.0) {
            qDebug() << "Frequency out of valid range (30Hz-180MHz)";
            return false;
        }

        try {
            auto collection = m_mongoClient["wavelengthDB"]["activeWavelengths"];
            auto document = bsoncxx::builder::basic::document{};
            document.append(bsoncxx::builder::basic::kvp("frequency", frequency));
            auto filter = document.view();
            auto result = collection.find_one(filter);

            bool available = !result;
            qDebug() << "Checking if frequency" << frequency << "is available:" << available;
            return available;
        }
        catch (const std::exception& e) {
            qDebug() << "MongoDB error:" << e.what();
            return false;
        }
    }

    bool getWavelengthDetails(double frequency, QString& name, bool& isPasswordProtected) {
        try {
            auto collection = m_mongoClient["wavelengthDB"]["activeWavelengths"];
            auto document = bsoncxx::builder::basic::document{};
            document.append(bsoncxx::builder::basic::kvp("frequency", frequency));
            auto filter = document.view();
            auto result = collection.find_one(filter);
            
            if (!result) {
                qDebug() << "Wavelength" << frequency << "not found in database";
                return false;
            }
            
            auto doc = result->view();
            isPasswordProtected = doc["isPasswordProtected"].get_bool().value;
            name = QString::fromStdString(static_cast<std::string>(doc["name"].get_string().value));
            
            return true;
        }
        catch (const std::exception& e) {
            qDebug() << "MongoDB error when getting wavelength details:" << e.what();
            return false;
        }
    }

    bool isConnected() const {
        return m_isConnected;
    }

private:
    DatabaseManager(QObject* parent = nullptr) 
        : QObject(parent), m_isConnected(false) {
        try {
            static mongocxx::instance instance{};
            m_mongoClient = mongocxx::client{mongocxx::uri{"mongodb+srv://wolfiksw:PrincePolo1@testcluster.taymr.mongodb.net/"}};
            
            m_isConnected = true;
            
            qDebug() << "Connected to MongoDB successfully";
        }
        catch (const std::exception& e) {
            qDebug() << "Failed to connect to MongoDB:" << e.what();
            m_isConnected = false;
        }
    }

    ~DatabaseManager() {}

    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    mongocxx::client m_mongoClient;
    bool m_isConnected;
};

#endif // DATABASE_MANAGER_H