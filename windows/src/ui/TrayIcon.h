#pragma once

#include <QIcon>
#include <QImage>

namespace eink::ui {

QImage bookPagesTrayImage(int size, bool lightBackground);
QIcon bookPagesTrayIcon(bool lightBackground);
bool systemTrayUsesLightBackground();

} // namespace eink::ui
