#pragma once

#include "AppState.h"

#include <QString>

namespace eink {

class SettingsStore {
public:
    explicit SettingsStore(QString path = {});
    AppSettings load() const;
    void save(const AppSettings &settings) const;
    QString path() const { return m_path; }

private:
    QString m_path;
};

} // namespace eink
