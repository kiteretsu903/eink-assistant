#pragma once

#include "core/IccProfile.h"
#include "platform/PlatformServices.h"

#include <QHash>
#include <QTemporaryDir>
#include <memory>
#include <windows.h>

namespace eink {

class WindowsPlatformServices : public PlatformServices {
public:
    WindowsPlatformServices();
    ~WindowsPlatformServices() override;

    ApplyResult recoverInterruptedColorState() override;
    QVector<DisplayInfo> displays() override;
    ApplyResult applyToneCurve(const DisplayInfo &, const ToneCurve &) override;
    ApplyResult restoreToneCurve(const DisplayInfo &) override;
    ApplyResult applyColor(const DisplayInfo &, double saturation, const RgbBalance &) override;
    ApplyResult restoreColor(const DisplayInfo &) override;
    ApplyResult setDitheringDisabled(const DisplayInfo &, bool disabled) override;
    bool visualEffectsReduced() const override;
    ApplyResult setVisualEffectsReduced(bool reduced) override;
    bool windowsLightModeAvailable() const override;
    bool windowsLightModeEnabled() const override;
    ApplyResult setWindowsLightModeEnabled(bool enabled) override;
    ApplyResult setLaunchAtLogin(bool enabled) override;
    bool launchAtLogin() const override;
    void openVisualEffectsSettings() override;
    ApplyResult recoverInterruptedNightLightState() override;
    bool nightLightControlAvailable() const override;
    bool nightLightEnabled() const override;
    ApplyResult setNightLightDisabled(bool disabled) override;
    ApplyResult restoreNightLightState() override;
    bool nightLightAvailable() const override;
    void openNightLightSettings() override;
    bool saturationPlatformAvailable() const override;
    QString platformDiagnostic() const override;
    void shutdown() override;

    static int runColorBroker(const QString &pipeName);
    ApplyResult setAcmForDiagnostics(const DisplayInfo &display, bool enabled) { return setAcm(display,enabled); }
    QString defaultProfileForDiagnostics(const DisplayInfo &display) const { return defaultProfile(display); }
    QStringList generatedProfilesForDiagnostics(const DisplayInfo &display) const;
    QStringList installedGeneratedProfilesForDiagnostics() const;
    ApplyResult cleanupGeneratedColorForDiagnostics();
    QString registeredLaunchTaskXmlForDiagnostics() const;

private:
    struct ColorState {
        QString previousProfile;
        QString currentProfile;
        bool captured = false;
        bool acmOriginallyEnabled = false;
        bool acmChanged = false;
        int scope = 1;
    };

    static quint32 windowsBuild();
    static QString errorMessage(const QString &operation, DWORD code = GetLastError());
    static bool queryDisplayPath(const QString &deviceName, LUID *adapter, UINT32 *sourceId, UINT32 *targetId,
                                 bool *builtIn = nullptr, QString *friendlyName = nullptr);
    static bool queryAcm(const DisplayInfo &, bool *supported, bool *enabled, QString *error = nullptr);
    static bool queryWindows10Mhc2(const DisplayInfo &, bool *supported, bool *verified, QString *error = nullptr);
    static bool modernColorProfileApisAvailable();
    static ApplyResult setAcm(const DisplayInfo &, bool enabled);
    static QString defaultProfile(const DisplayInfo &);
    static int profileScope(const DisplayInfo &);
    static QString colorDirectory();
    static QString nightLightRecoveryFilePath();
    static ApplyResult readNightLightState(QByteArray *data);
    static ApplyResult writeNightLightState(const QByteArray &data);
    static bool readNightLightRecovery(QByteArray *data);
    static ApplyResult writeNightLightRecovery(const QByteArray &data);
    static void clearNightLightRecovery();
    static QString recoveryFilePath();
    static QString recoveryGroup(const DisplayInfo &);
    static QString profilePrefix(const DisplayInfo &);
    static bool isGeneratedProfile(const QString &profile);
    static QStringList associatedProfiles(const DisplayInfo &, int scope, QString *error = nullptr);
    ApplyResult removeProfileAssociation(const DisplayInfo &, const QString &profile, int scope = -1);
    ApplyResult setDefaultProfile(const DisplayInfo &, const QString &profile, int scope = -1);
    ApplyResult writeRecoveryRecord(const DisplayInfo &, const ColorState &);
    bool readRecoveryRecord(const DisplayInfo &, ColorState *) const;
    void clearRecoveryRecord(const DisplayInfo &);
    ApplyResult removeGeneratedAssociations(const DisplayInfo &, QStringList *removed = nullptr);
    ApplyResult uninstallGeneratedProfiles(const QStringList &profiles);

    ApplyResult ensureBroker();
    ApplyResult brokerCommand(const QString &command, QString *response = nullptr);
    IccBaseProfile baseProfileFor(const DisplayInfo &, const ColorState &state) const;
    void closeBroker();

    QHash<QString, QByteArray> m_originalGamma;
    QHash<QString, ColorState> m_colorStates;
    QHash<QString, IccBaseProfile> m_baseProfiles;
    std::unique_ptr<QTemporaryDir> m_profileTemp;
    HANDLE m_pipe = INVALID_HANDLE_VALUE;
    HANDLE m_brokerProcess = nullptr;
    QString m_pipeName;
    QString m_profileSessionId;
    quint64 m_profileSequence = 0;
    bool m_recoveryComplete = false;
    QByteArray m_originalNightLightState;
    bool m_nightLightOwned = false;
    bool m_shutdown = false;
};

} // namespace eink
