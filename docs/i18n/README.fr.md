# E-Ink Assistant

<!-- BEGIN README LANGUAGES -->
<p align="center">
  <a href="../../README.md" lang="en" dir="ltr">English</a> &nbsp;·&nbsp;
  <a href="README.zh-Hans.md" lang="zh-Hans" dir="ltr">简体中文</a> &nbsp;·&nbsp;
  <a href="README.zh-Hant.md" lang="zh-Hant" dir="ltr">繁體中文</a> &nbsp;·&nbsp;
  <a href="README.ja.md" lang="ja" dir="ltr">日本語</a> &nbsp;·&nbsp;
  <a href="README.ko.md" lang="ko" dir="ltr">한국어</a> &nbsp;·&nbsp;
  <a href="README.es.md" lang="es" dir="ltr">Español</a> &nbsp;·&nbsp;
  <b lang="fr" dir="ltr">Français</b> &nbsp;·&nbsp;
  <a href="README.de.md" lang="de" dir="ltr">Deutsch</a> &nbsp;·&nbsp;
  <a href="README.pt-BR.md" lang="pt-BR" dir="ltr">Português (Brasil)</a> &nbsp;·&nbsp;
  <a href="README.ru.md" lang="ru" dir="ltr">Русский</a> &nbsp;·&nbsp;
  <a href="README.ar.md" lang="ar" dir="rtl">العربية</a> &nbsp;·&nbsp;
  <a href="README.hi.md" lang="hi" dir="ltr">हिन्दी</a>
</p>
<!-- END README LANGUAGES -->

<p align="center">
  <img src="../../Resources/AppIcon.png" alt="Icône de l’application E-Ink Assistant" width="128">
</p>

**Réglez vos écrans à encre électronique noir et blanc ou couleur sous macOS et Windows.**

[Visiter le site du produit](https://kiteretsu903.github.io/eink-assistant/fr/)

E-Ink Assistant ajuste le contraste du texte, les détails dans les ombres et les
couleurs des écrans à encre électronique de votre choix. Les autres écrans restent
inchangés. La version macOS se trouve dans la barre des menus ; la version Windows,
dans la zone de notification.

[Télécharger macOS 2.6](https://github.com/kiteretsu903/eink-assistant/releases/download/macos-v2.6-windows-v1.2/E-Ink-Assistant-v2.6.dmg) ·
[Télécharger Windows 1.2](https://github.com/kiteretsu903/eink-assistant/releases/download/macos-v2.6-windows-v1.2/E-Ink-Assistant-Windows-1.2-Setup.exe) ·
[Voir toutes les versions](https://github.com/kiteretsu903/eink-assistant/releases)

Gratuit, open source et sous licence MIT.

## Fonctionnalités et configuration requise

| Fonctionnalité | macOS | Windows |
|---|---|---|
| Systèmes pris en charge | **macOS 14 ou version ultérieure**<br>Apple silicon uniquement | **De Windows 7 SP1 à Windows 11**<br>Ordinateurs x64 |
| Emplacement de l’application | Barre des menus | Zone de notification |
| Choisir des écrans à encre électronique précis | Oui. Les autres écrans restent inchangés. | Comme sous macOS |
| Contraste du texte | Quatre niveaux : Moyen, Fort, Net, Dense | Comme sous macOS |
| Amélioration vidéo | Trois niveaux : Léger, Moyen, Fort | Comme sous macOS |
| Courbe avancée et préréglages | Éditeur de courbe en direct et cinq préréglages nommés | Comme sous macOS |
| Saturation et RGB | Profil colorimétrique par écran ; saturation de 0%–300% et RGB de 0%–200% | Disponible sur les systèmes Windows 10 2004 et Windows 11 21H2+ compatibles ; la méthode disponible dépend du système et du matériel |
| Réduire le scintillement | Disponible sur les écrans externes compatibles ; s’active lorsqu’un écran est marqué comme étant à encre électronique | Non disponible. Windows n’offre pas de réglage public unifié du tramage par écran, et la plupart des systèmes Windows n’en ont probablement pas besoin. |
| Réduire la transparence et les animations | Disponible via un assistant nécessitant une seule confirmation de l’utilisateur | Disponible à partir de Windows 7 SP1 via les API système compatibles |
| Mode clair du système | Non modifié | Mode clair de Windows limité à la session sous Windows 10 1903+ |
| Night Shift / Éclairage nocturne | Exclusion de Night Shift et True Tone par écran ; nécessite une autorisation administrateur et une reconnexion | Paramètres d’éclairage nocturne à partir de Windows 10 1703+ ; commande directe de désactivation sous Windows 11 24H2+ |
| Écrans en recopie / dupliqués | Les écrans physiques en recopie restent sélectionnables individuellement | Les courbes tonales affectent la source partagée ; la saturation et RGB nécessitent le mode Étendre |
| Rétablissement des modifications | Les courbes temporaires, profils colorimétriques et réglages de tramage sont rétablis à la fermeture ; l’exclusion de Night Shift / True Tone persiste | Les modifications temporaires du Gamma, des couleurs, des effets visuels et de l’éclairage nocturne sont rétablies à la fermeture ; les couleurs et l’éclairage nocturne sont aussi restaurés après un arrêt anormal |
| Ouvrir à la connexion | Pris en charge | Pris en charge |
| Langues de l’interface | Anglais, chinois simplifié, chinois traditionnel, japonais | Comme sous macOS |
| Accès administrateur | Uniquement pour l’exclusion facultative de Night Shift / True Tone | Requis par le programme d’installation et l’application |

[Détails macOS](../../macos/README.md) ·
[Compatibilité et configuration Windows](../../WINDOWS.md) ·
[Historique macOS](../../CHANGELOG.md) ·
[Historique Windows](../../windows/CHANGELOG.md)

<p align="center">
  <img src="../../docs/en/app-main-v2-1.png" alt="E-Ink Assistant v2.1 en anglais" width="440">
</p>

## Réglages

| Réglage | Utilisation | Effet |
|---|---|---|
| Contraste du texte | Lecture | Assombrit le texte pâle avec les niveaux Moyen, Fort, Net et Dense. Les niveaux plus forts sacrifient les détails gris au profit de contours plus durs. |
| Amélioration vidéo | Photos et vidéos | Révèle les détails dans les ombres avec les niveaux Léger, Moyen et Fort. Désactivez-la pour lire, car elle éclaircit aussi le texte sombre. |
| Saturation et RGB | Encre électronique couleur | Propose six préréglages de saturation, un curseur de saturation de 0%–300% et une correction RGB de 0%–200% lorsque la plateforme le permet. |
| Réduire le scintillement | Écrans macOS compatibles | Supprime le scintillement visible dû au tramage et s’active automatiquement pour les écrans marqués comme étant à encre électronique. |
| Night Shift et True Tone | Écrans affectés par les changements de température de couleur | Exclut l’écran macOS sélectionné. Cette opération nécessite une autorisation administrateur et une reconnexion de l’écran ; le réglage persiste après la fermeture. |
| Réduire la transparence et les animations | Écrans à rafraîchissement lent | Simplifie les effets visuels du système. macOS utilise un assistant nécessitant une seule confirmation de l’utilisateur. |
| Courbes avancées | Réglages propres à chaque écran | Ajuste le coude, le Gamma, le point noir et le point blanc avec un graphique en direct et cinq préréglages nommés. |

<p align="center">
  <img src="../../docs/en/text-contrast-editorial.png" alt="Illustration du contraste du texte avant et après" width="31%">
  <img src="../../docs/en/video-enhance-editorial.png" alt="Illustration de l’amélioration vidéo avant et après" width="31%">
  <img src="../../docs/en/saturation-editorial.png" alt="Illustration de la saturation avant et après" width="31%">
</p>

> Ces images illustrent les réglages. Les résultats dépendent de l’écran et du
> contenu source.

## Installation

### macOS 14+, Apple silicon

1. Téléchargez le DMG macOS 2.6 avec le lien ci-dessus.
2. Ouvrez-le et faites glisser **E-Ink Assistant** dans **Applications**.
3. Essayez d’ouvrir l’application une première fois. Si macOS la bloque, ouvrez
   **Réglages Système → Confidentialité et sécurité**, puis sélectionnez
   **Ouvrir quand même**.

Ce logiciel est développé indépendamment et n’est actuellement pas disponible
sur l’App Store. macOS affiche un avertissement indiquant qu’il ne peut pas
vérifier l’application lors de la première ouverture. Le code est entièrement
open source : vous pouvez l’examiner avant de décider de l’utiliser.

Si **Ouvrir quand même** n’apparaît pas après avoir déplacé l’application dans
Applications, ouvrez Terminal et exécutez :

```
xattr -dr com.apple.quarantine "/Applications/E-Ink Assistant.app"
```

### De Windows 7 SP1 à Windows 11, x64

1. Téléchargez le programme d’installation Windows 1.2 avec le lien ci-dessus.
2. Exécutez le programme d’installation et acceptez la demande d’autorisation administrateur.
3. Ouvrez E-Ink Assistant depuis le menu Démarrer ou la zone de notification.

Consultez [WINDOWS.md](../../WINDOWS.md) pour connaître précisément les
fonctionnalités disponibles selon la version de Windows, le GPU, le pilote et la
connexion de l’écran.

## Utilisation

<p align="center">
  <img src="../../docs/en/app-displays-v2-1.png" alt="Sélection des écrans dans E-Ink Assistant v2.1" width="440">
</p>

1. Ouvrez l’application depuis la barre des menus de macOS ou la zone de notification de Windows.
2. Marquez chaque écran à encre électronique noir et blanc ou couleur que vous souhaitez régler.
3. Commencez par choisir un contraste matériel équilibré dans le menu du moniteur lui-même.
4. Choisissez le contraste du texte pour lire ou l’amélioration vidéo pour les médias, sans activer les deux.
5. Sur l’encre électronique couleur, ajustez la saturation et RGB lorsque la plateforme le permet.

**Les réglages d’affichage initiaux sont rétablis lorsque vous quittez
l’application**, et vos réglages sont réappliqués au lancement. Activez
**Ouvrir à la connexion** pour un démarrage automatique.

## Configuration de l’écran

Choisissez un contraste équilibré dans le menu du moniteur lui-même avant de
régler l’application. Les préréglages intégrés ont été ajustés à l’œil sur un
**Bigme B251 Pro** (R2 FW V2.0) avec les réglages suivants : **mode Web,
Gamma matériel niveau 3, contraste 50, mode de restauration des couleurs désactivé**.
Un écran noir et blanc ou un autre modèle couleur nécessitera ses propres valeurs.
Le mode avancé donne accès à la courbe complète et les réglages sont enregistrés
séparément pour chaque écran.

La réduction du scintillement est réservée à Apple Silicon et est masquée
lorsqu’elle n’est pas prise en charge.

<details>
<summary>Assistant macOS de réduction de la transparence et des animations</summary>

À la première utilisation, vous devez confirmer **Ajouter un raccourci** dans
l’application Raccourcis d’Apple. L’assistant intégré accepte uniquement les
commandes textuelles exactes `on` et `off`, ne produit aucune sortie et
n’apparaît pas dans la feuille de partage, Spotlight, les actions rapides ni
l’interface de l’écran verrouillé. Il peut s’exécuter lorsque le Mac est
verrouillé. L’application ne répertorie ni n’inspecte vos autres raccourcis.

Le mode automatique active les deux réglages lorsqu’un écran marqué comme étant
à encre électronique est connecté et les désactive après la déconnexion du
dernier écran marqué. Quitter l’application les désactive également.

</details>

## Documentation du projet

- [CHANGELOG.md](../../CHANGELOG.md) : modifications de chaque version
- [TECHNICAL.md](../../TECHNICAL.md) : implémentation, mesures et approches qui
  ne fonctionnent *pas* sur les versions modernes de macOS
- [mac-saturation](https://github.com/kiteretsu903/mac-saturation) : étude du
  mécanisme de couleur et outil en ligne de commande pour exporter des profils

## Licence et crédits

MIT, voir [LICENSE](../../LICENSE).

**La réduction du scintillement est basée sur
[Stillcolor](https://github.com/aiaf/Stillcolor) d’Abdullah Arif** (MIT).
Stillcolor a découvert que le tramage d’affichage pouvait être désactivé via la
propriété `enableDither` du registre d’E/S. Ce projet réimplémente cette idée
par écran ; le mérite de cette découverte revient à Stillcolor. Merci.

Mentions complètes dans [THIRD-PARTY-NOTICES.md](../../THIRD-PARTY-NOTICES.md).
