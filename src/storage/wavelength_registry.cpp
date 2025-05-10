#include "wavelength_registry.h"

bool WavelengthRegistry::AddWavelength(const QString &frequency, const WavelengthInfo &info) {
    if (HasWavelength(frequency)) {
        return false;
    }

    wavelengths_[frequency] = info;
    wavelengths_[frequency].frequency = frequency;

    if (pending_registrations_.contains(frequency)) {
        pending_registrations_.remove(frequency);
    }

    emit wavelengthAdded(frequency);
    return true;
}

bool WavelengthRegistry::RemoveWavelength(const QString &frequency) {
    if (!HasWavelength(frequency)) {
        qDebug() << "[REGISTRY] Cannot remove - wavelength" << frequency << "does not exist";
        return false;
    }

    if (active_wavelength_ == frequency) {
        active_wavelength_ = -1;
    }

    wavelengths_.remove(frequency);

    emit wavelengthRemoved(frequency);
    return true;
}

WavelengthInfo WavelengthRegistry::GetWavelengthInfo(const QString &frequency) const {
    if (HasWavelength(frequency)) {
        return wavelengths_[frequency];
    }
    return WavelengthInfo();
}

bool WavelengthRegistry::UpdateWavelength(const QString &frequency, const WavelengthInfo &info) {
    if (!HasWavelength(frequency)) {
        qDebug() << "[REGISTRY] Cannot update - wavelength" << frequency << "does not exist.";
        return false;
    }

    wavelengths_[frequency] = info;
    wavelengths_[frequency].frequency = frequency;

    emit wavelengthUpdated(frequency);
    return true;
}

void WavelengthRegistry::SetActiveWavelength(const QString &frequency) {
    if (frequency != -1 && !HasWavelength(frequency)) {
        qDebug() << "[REGISTRY] Cannot set active - wavelength" << frequency << "does not exist.";
        return;
    }

    const QString previousActive = active_wavelength_;
    active_wavelength_ = frequency;

    emit activeWavelengthChanged(previousActive, frequency);
}

bool WavelengthRegistry::AddPendingRegistration(const QString &frequency) {
    if (HasWavelength(frequency) || pending_registrations_.contains(frequency)) {
        qDebug() << "[REGISTRY] Cannot add pending registration - wavelength"
                << frequency << "already exists or is pending.";
        return false;
    }

    pending_registrations_.insert(frequency);
    return true;
}

bool WavelengthRegistry::RemovePendingRegistration(const QString &frequency) {
    if (pending_registrations_.contains(frequency)) {
        pending_registrations_.remove(frequency);
        return true;
    }
    return false;
}

QPointer<QWebSocket> WavelengthRegistry::GetWavelengthSocket(const QString &frequency) const {
    if (HasWavelength(frequency)) {
        return wavelengths_[frequency].socket;
    }
    return nullptr;
}

void WavelengthRegistry::SetWavelengthSocket(const QString &frequency, const QPointer<QWebSocket> &socket) {
    if (HasWavelength(frequency)) {
        wavelengths_[frequency].socket = socket;
    }
}

bool WavelengthRegistry::MarkWavelengthClosing(const QString &frequency, const bool closing) {
    if (!HasWavelength(frequency)) {
        return false;
    }

    wavelengths_[frequency].is_closing = closing;
    return true;
}

bool WavelengthRegistry::IsWavelengthClosing(const QString &frequency) const {
    if (HasWavelength(frequency)) {
        return wavelengths_[frequency].is_closing;
    }
    return false;
}

void WavelengthRegistry::ClearAllWavelengths() {
    QList<QString> frequencies = wavelengths_.keys();

    for (QString frequency: frequencies) {
        RemoveWavelength(frequency);
    }

    pending_registrations_.clear();
    active_wavelength_ = -1;
}
