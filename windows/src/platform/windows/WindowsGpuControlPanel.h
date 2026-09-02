#pragma once

#include "core/AppState.h"
#include "platform/PlatformServices.h"

#include <QStringList>
#include <QVector>

namespace eink::windows {

enum class GpuPanelLaunchKind {
    Unavailable,
    ShellApplication,
    Executable,
    ControlPanelApplet
};

struct GpuPanelLaunchCandidate {
    GpuPanelLaunchKind kind = GpuPanelLaunchKind::Unavailable;
    QString target;
    QString arguments;

    bool isValid() const { return kind != GpuPanelLaunchKind::Unavailable && !target.isEmpty(); }
};

struct RegisteredShellApplication {
    QString name;
    QString id;
};

struct GraphicsAdapterDetails {
    GraphicsVendor vendor = GraphicsVendor::Unknown;
    quint32 pciVendorId = 0;
    QString name;
};

GraphicsVendor graphicsVendorFromPciVendorId(quint32 vendorId);
GraphicsVendor graphicsVendorFromDeviceId(const QString &deviceId);
GraphicsAdapterDetails graphicsAdapterForLuid(qint32 highPart, quint32 lowPart);

QVector<GpuPanelLaunchCandidate> knownGpuControlPanelCandidates(
    GraphicsVendor vendor, const QString &programFiles, const QString &programFilesX86,
    const QString &windowsDirectory);
bool shellApplicationMatchesVendor(GraphicsVendor vendor, const RegisteredShellApplication &application);
GpuPanelLaunchCandidate selectGpuControlPanelCandidate(
    const QVector<GpuPanelLaunchCandidate> &candidates,
    const QStringList &registeredShellApplicationIds,
    const QStringList &existingFiles);

GpuPanelLaunchCandidate resolveGpuControlPanel(GraphicsVendor vendor);
bool gpuControlPanelAvailable(GraphicsVendor vendor);
ApplyResult launchGpuControlPanel(GraphicsVendor vendor);

} // namespace eink::windows
