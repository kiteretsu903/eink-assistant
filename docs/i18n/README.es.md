# E-Ink Assistant

<!-- BEGIN README LANGUAGES -->
<p align="center">
  <a href="../../README.md" lang="en" dir="ltr">English</a> &nbsp;·&nbsp;
  <a href="README.zh-Hans.md" lang="zh-Hans" dir="ltr">简体中文</a> &nbsp;·&nbsp;
  <a href="README.zh-Hant.md" lang="zh-Hant" dir="ltr">繁體中文</a> &nbsp;·&nbsp;
  <a href="README.ja.md" lang="ja" dir="ltr">日本語</a> &nbsp;·&nbsp;
  <a href="README.ko.md" lang="ko" dir="ltr">한국어</a> &nbsp;·&nbsp;
  <b lang="es" dir="ltr">Español</b> &nbsp;·&nbsp;
  <a href="README.fr.md" lang="fr" dir="ltr">Français</a> &nbsp;·&nbsp;
  <a href="README.de.md" lang="de" dir="ltr">Deutsch</a> &nbsp;·&nbsp;
  <a href="README.pt-BR.md" lang="pt-BR" dir="ltr">Português (Brasil)</a> &nbsp;·&nbsp;
  <a href="README.ru.md" lang="ru" dir="ltr">Русский</a> &nbsp;·&nbsp;
  <a href="README.ar.md" lang="ar" dir="rtl">العربية</a> &nbsp;·&nbsp;
  <a href="README.hi.md" lang="hi" dir="ltr">हिन्दी</a>
</p>
<!-- END README LANGUAGES -->

<p align="center">
  <img src="../../Resources/AppIcon.png" alt="Icono de la app E-Ink Assistant" width="128">
</p>

**Ajusta pantallas de tinta electrónica en blanco y negro y en color en macOS y Windows.**

[Visita el sitio web del producto](https://kiteretsu903.github.io/eink-assistant/es/)

E-Ink Assistant ajusta el contraste del texto, el detalle en las sombras y el color
en las pantallas de tinta electrónica que elijas. Las demás pantallas no cambian.
La edición para macOS se ejecuta en la barra de menús; la edición para Windows,
en la bandeja del sistema.

[Descargar macOS 2.6](https://github.com/kiteretsu903/eink-assistant/releases/download/macos-v2.6-windows-v1.2/E-Ink-Assistant-v2.6.dmg) ·
[Descargar Windows 1.2](https://github.com/kiteretsu903/eink-assistant/releases/download/macos-v2.6-windows-v1.2/E-Ink-Assistant-Windows-1.2-Setup.exe) ·
[Ver todas las versiones](https://github.com/kiteretsu903/eink-assistant/releases)

Gratuito, de código abierto y con licencia MIT.

## Funciones y requisitos del sistema

| Función | macOS | Windows |
|---|---|---|
| Sistemas compatibles | **macOS 14 o posterior**<br>Solo Apple silicon | **Desde Windows 7 SP1 hasta Windows 11**<br>Equipos x64 |
| Dónde se ejecuta la app | Barra de menús | Bandeja del sistema |
| Seleccionar pantallas de tinta electrónica concretas | Sí. Las demás pantallas no cambian. | Igual que en macOS |
| Contraste del texto | Cuatro niveles: Medio, Fuerte, Nítido, Sólido | Igual que en macOS |
| Mejora de vídeo | Tres niveles: Sutil, Medio, Fuerte | Igual que en macOS |
| Curva avanzada y ajustes predefinidos | Editor de curvas en tiempo real y cinco ajustes predefinidos con nombre | Igual que en macOS |
| Saturación y RGB | Perfil de color por pantalla; saturación del 0%–300% y RGB del 0%–200% | Disponible en sistemas Windows 10 2004 y Windows 11 21H2+ compatibles; el método disponible depende del sistema y del hardware |
| Reducir parpadeo | Disponible en pantallas externas compatibles; se activa al marcar una pantalla como tinta electrónica | No disponible. Windows no tiene un control público unificado del tramado por pantalla, y probablemente la mayoría de los sistemas Windows no lo necesiten. |
| Reducir transparencia y movimiento | Disponible mediante un asistente que el usuario confirma una sola vez | Disponible desde Windows 7 SP1 mediante API compatibles del sistema |
| Modo claro del sistema | No se modifica | Modo claro de Windows solo durante la sesión en Windows 10 1903+ |
| Night Shift / Luz nocturna | Exclusión de Night Shift y True Tone por pantalla; requiere aprobación de administrador y volver a conectar la pantalla | Ajustes de Luz nocturna desde Windows 10 1703+; control directo Desactivar Luz nocturna en Windows 11 24H2+ |
| Pantallas en espejo / duplicadas | Las pantallas físicas en espejo siguen siendo seleccionables individualmente | Las curvas tonales afectan al origen compartido; Saturación y RGB requieren el modo Extender |
| Restaurar cambios | Las curvas, los perfiles de color y el tramado temporales se restauran al salir; la exclusión de Night Shift / True Tone es permanente | Los cambios temporales de gamma, color, efectos visuales y Luz nocturna se restauran al salir; el color y la Luz nocturna también se recuperan tras un cierre anormal |
| Abrir al iniciar sesión | Compatible | Compatible |
| Idiomas de la interfaz | Inglés, chino simplificado, chino tradicional, japonés | Igual que en macOS |
| Acceso de administrador | Solo para la exclusión opcional de Night Shift / True Tone | Necesario para el instalador y la app |

[Detalles de macOS](../../macos/README.md) ·
[Compatibilidad y configuración de Windows](../../WINDOWS.md) ·
[Historial de cambios de macOS](../../CHANGELOG.md) ·
[Historial de cambios de Windows](../../windows/CHANGELOG.md)

<p align="center">
  <img src="../../docs/en/app-main-v2-1.png" alt="E-Ink Assistant v2.1 en inglés" width="440">
</p>

## Controles

| Control | Para qué usarlo | Qué hace |
|---|---|---|
| Contraste del texto | Lectura | Oscurece el texto tenue con los niveles Medio, Fuerte, Nítido y Sólido. Los niveles más intensos sacrifican detalles en gris para obtener bordes más duros. |
| Mejora de vídeo | Fotos y vídeos | Revela el detalle en las sombras con los niveles Sutil, Medio y Fuerte. Desactívala para leer, ya que también aclara el texto oscuro. |
| Saturación y RGB | Tinta electrónica en color | Ofrece seis ajustes predefinidos de saturación, un deslizador de saturación del 0%–300% y corrección RGB del 0%–200% cuando la plataforma lo permite. |
| Reducir parpadeo | Pantallas compatibles con macOS | Detiene el parpadeo visible causado por el tramado y se activa automáticamente en las pantallas marcadas como tinta electrónica. |
| Night Shift y True Tone | Pantallas afectadas por cambios de temperatura de color | Excluye la pantalla de macOS seleccionada. Requiere aprobación de administrador y volver a conectar la pantalla; el ajuste se mantiene al salir. |
| Reducir transparencia y movimiento | Paneles de actualización lenta | Simplifica los efectos visuales del sistema. macOS utiliza un asistente que el usuario confirma una sola vez. |
| Curvas avanzadas | Ajuste específico de cada panel | Ajusta el punto de transición, gamma, punto negro y punto blanco con un gráfico en tiempo real y cinco ajustes predefinidos con nombre. |

<p align="center">
  <img src="../../docs/en/text-contrast-editorial.png" alt="Ejemplo del contraste del texto antes y después" width="31%">
  <img src="../../docs/en/video-enhance-editorial.png" alt="Ejemplo de la mejora de vídeo antes y después" width="31%">
  <img src="../../docs/en/saturation-editorial.png" alt="Ejemplo de saturación antes y después" width="31%">
</p>

> Estas imágenes ilustran los controles. Los resultados dependen del panel y del
> contenido original.

## Instalación

### macOS 14+, Apple silicon

1. Descarga el DMG de macOS 2.6 mediante el enlace anterior.
2. Ábrelo y arrastra **E-Ink Assistant** a **Aplicaciones**.
3. Intenta abrir la app una vez. Si macOS la bloquea, abre **Ajustes del Sistema →
   Privacidad y seguridad** y selecciona **Abrir igualmente**.

Este software se desarrolla de forma independiente y actualmente no está en el
App Store. macOS mostrará un aviso de que no puede verificarlo la primera vez
que lo abras. Todo el código es abierto, así que puedes revisarlo antes de
decidir si lo usas.

Si no aparece **Abrir igualmente** después de mover la app a Aplicaciones, abre
Terminal y ejecuta:

```
xattr -dr com.apple.quarantine "/Applications/E-Ink Assistant.app"
```

### Desde Windows 7 SP1 hasta Windows 11, x64

1. Descarga el instalador de Windows 1.2 mediante el enlace anterior.
2. Ejecuta el instalador y acepta la solicitud de permisos de administrador.
3. Abre E-Ink Assistant desde el menú Inicio o la bandeja del sistema.

Consulta [WINDOWS.md](../../WINDOWS.md) para conocer la disponibilidad exacta de
cada función según la versión de Windows, la GPU, el controlador y la conexión
de pantalla.

## Uso

<p align="center">
  <img src="../../docs/en/app-displays-v2-1.png" alt="Selección de pantallas en E-Ink Assistant v2.1" width="440">
</p>

1. Abre la app desde la barra de menús de macOS o la bandeja del sistema de Windows.
2. Marca cada pantalla de tinta electrónica en blanco y negro o en color que quieras ajustar.
3. Primero establece un contraste de hardware equilibrado en el menú del propio monitor.
4. Elige Contraste del texto para leer o Mejora de vídeo para contenido multimedia, no ambos.
5. En tinta electrónica en color, ajusta Saturación y RGB cuando la plataforma lo permita.

**Los ajustes de pantalla se restauran al salir** y se vuelven a aplicar al abrir
la app. Activa **Abrir al iniciar sesión** para que se abra automáticamente.

## Configuración de la pantalla

Establece un contraste equilibrado en el menú del propio monitor antes de ajustar
la app. Los ajustes predefinidos incluidos se calibraron visualmente en un
**Bigme B251 Pro** (R2 FW V2.0) con **Web Mode, Hardware Gamma Level 3,
Contrast 50, Color Restore Mode desactivado**. Un panel en blanco y negro u otro
modelo en color necesitará sus propios valores. El modo Avanzado permite ajustar
la curva completa, y la configuración se guarda por separado para cada pantalla.

Reducir parpadeo solo está disponible en Apple Silicon y se oculta cuando no es compatible.

<details>
<summary>Asistente de macOS para Reducir transparencia y movimiento</summary>

El primer uso te pide confirmar **Añadir atajo** en la app Atajos de Apple.
El asistente incluido solo acepta los comandos exactos de Texto `on` y `off`,
no genera salida y no aparece en la hoja para compartir, Spotlight, Acciones
rápidas ni la interfaz de la pantalla de bloqueo. Puede ejecutarse con el Mac
bloqueado. La app no enumera ni inspecciona tus otros atajos.

El modo automático activa ambos ajustes al conectar una pantalla marcada como
tinta electrónica y los desactiva al desconectar la última pantalla marcada.
Al salir de la app también se desactivan.

</details>

## Documentación del proyecto

- [CHANGELOG.md](../../CHANGELOG.md): cambios de cada versión
- [TECHNICAL.md](../../TECHNICAL.md): implementación, mediciones y métodos que
  *no* funcionan en macOS moderno
- [mac-saturation](https://github.com/kiteretsu903/mac-saturation): investigación
  del mecanismo de color y una herramienta de línea de comandos para exportar perfiles

## Licencia y créditos

MIT, consulta [LICENSE](../../LICENSE).

**Reducir parpadeo se basa en [Stillcolor](https://github.com/aiaf/Stillcolor), de
Abdullah Arif** (MIT). Stillcolor descubrió que el tramado de pantalla se puede
desactivar mediante la propiedad `enableDither` del registro de E/S. Este proyecto
implementa esa idea por pantalla; el mérito del descubrimiento corresponde a
Stillcolor. Gracias.

Avisos completos en [THIRD-PARTY-NOTICES.md](../../THIRD-PARTY-NOTICES.md).
