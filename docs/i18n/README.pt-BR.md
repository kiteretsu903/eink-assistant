# E-Ink Assistant

<!-- BEGIN README LANGUAGES -->
<p align="center">
  <a href="../../README.md" lang="en" dir="ltr">English</a> &nbsp;·&nbsp;
  <a href="README.zh-Hans.md" lang="zh-Hans" dir="ltr">简体中文</a> &nbsp;·&nbsp;
  <a href="README.zh-Hant.md" lang="zh-Hant" dir="ltr">繁體中文</a> &nbsp;·&nbsp;
  <a href="README.ja.md" lang="ja" dir="ltr">日本語</a> &nbsp;·&nbsp;
  <a href="README.ko.md" lang="ko" dir="ltr">한국어</a> &nbsp;·&nbsp;
  <a href="README.es.md" lang="es" dir="ltr">Español</a> &nbsp;·&nbsp;
  <a href="README.fr.md" lang="fr" dir="ltr">Français</a> &nbsp;·&nbsp;
  <a href="README.de.md" lang="de" dir="ltr">Deutsch</a> &nbsp;·&nbsp;
  <b lang="pt-BR" dir="ltr">Português (Brasil)</b> &nbsp;·&nbsp;
  <a href="README.ru.md" lang="ru" dir="ltr">Русский</a> &nbsp;·&nbsp;
  <a href="README.ar.md" lang="ar" dir="rtl">العربية</a> &nbsp;·&nbsp;
  <a href="README.hi.md" lang="hi" dir="ltr">हिन्दी</a>
</p>
<!-- END README LANGUAGES -->

<p align="center">
  <img src="../../Resources/AppIcon.png" alt="Ícone do app E-Ink Assistant" width="128">
</p>

**Ajuste monitores e-ink em preto e branco e coloridos no macOS e no Windows.**

[Visite o site do produto](https://kiteretsu903.github.io/eink-assistant/pt-BR/)

O E-Ink Assistant ajusta o contraste do texto, os detalhes nas sombras e as cores
nos monitores e-ink que você escolher. Os outros monitores permanecem inalterados.
A edição para macOS funciona na barra de menus; a edição para Windows funciona
na bandeja do sistema.

[Baixar macOS 2.6](https://github.com/kiteretsu903/eink-assistant/releases/download/macos-v2.6-windows-v1.2/E-Ink-Assistant-v2.6.dmg) ·
[Baixar Windows 1.2](https://github.com/kiteretsu903/eink-assistant/releases/download/macos-v2.6-windows-v1.2/E-Ink-Assistant-Windows-1.2-Setup.exe) ·
[Ver todas as versões](https://github.com/kiteretsu903/eink-assistant/releases)

Gratuito, de código aberto e sob a licença MIT.

## Recursos e requisitos do sistema

| Recurso | macOS | Windows |
|---|---|---|
| Sistemas compatíveis | **macOS 14 ou posterior**<br>Somente Apple silicon | **Windows 7 SP1 ao Windows 11**<br>Computadores x64 |
| Onde o app é executado | Barra de menus | Bandeja do sistema |
| Escolher monitores e-ink específicos | Sim. Os outros monitores permanecem inalterados. | Igual ao macOS |
| Contraste do texto | Quatro níveis: Médio, Forte, Nítido, Sólido | Igual ao macOS |
| Melhoria de vídeo | Três níveis: Sutil, Médio, Forte | Igual ao macOS |
| Curva avançada e predefinições | Editor de curva em tempo real e cinco predefinições nomeadas | Igual ao macOS |
| Saturação e RGB | Perfil de cores por monitor; saturação de 0%–300% e RGB de 0%–200% | Disponível em sistemas compatíveis com Windows 10 2004 e Windows 11 21H2+; o método disponível depende do sistema e do hardware |
| Reduzir tremulação | Disponível em monitores externos compatíveis; ativa quando um monitor é marcado como e-ink | Não disponível. O Windows não tem um controle público unificado de dithering por monitor, e a maioria dos sistemas Windows provavelmente não precisa dele. |
| Reduzir transparência e movimento | Disponível por meio de um auxiliar confirmado uma única vez pelo usuário | Disponível a partir do Windows 7 SP1 por meio de APIs compatíveis do sistema |
| Modo claro do sistema | Não é alterado | Modo Claro do Windows apenas durante a sessão no Windows 10 1903+ |
| Night Shift / Luz noturna | Exclusão de Night Shift e True Tone por monitor; exige aprovação de administrador e reconexão | Configurações de Luz noturna a partir do Windows 10 1703+; controle direto de Desativar Luz noturna no Windows 11 24H2+ |
| Monitores espelhados / duplicados | Monitores físicos espelhados continuam selecionáveis individualmente | As curvas tonais afetam a origem compartilhada; Saturação e RGB exigem o modo Estender |
| Restaurar alterações | Curvas temporárias, perfis de cores e dithering são restaurados ao sair; a exclusão de Night Shift / True Tone é persistente | Alterações temporárias de Gamma, cor, aparência e Luz noturna são restauradas ao sair; cor e Luz noturna também são recuperadas após um encerramento anormal |
| Iniciar ao entrar | Compatível | Compatível |
| Idiomas da interface | Inglês, chinês simplificado, chinês tradicional, japonês | Igual ao macOS |
| Acesso de administrador | Somente para a exclusão opcional de Night Shift / True Tone | Exigido pelo instalador e pelo app |

[Detalhes do macOS](../../macos/README.md) ·
[Compatibilidade e configuração no Windows](../../WINDOWS.md) ·
[Histórico de alterações do macOS](../../CHANGELOG.md) ·
[Histórico de alterações do Windows](../../windows/CHANGELOG.md)

<p align="center">
  <img src="../../docs/en/app-main-v2-1.png" alt="E-Ink Assistant v2.1 em inglês" width="440">
</p>

## Controles

| Controle | Quando usar | O que faz |
|---|---|---|
| Contraste do texto | Leitura | Escurece textos claros com os níveis Médio, Forte, Nítido e Sólido. Níveis mais fortes trocam detalhes em cinza por bordas mais duras. |
| Melhoria de vídeo | Fotos e vídeos | Revela detalhes nas sombras com os níveis Sutil, Médio e Forte. Desative para leitura, pois também clareia o texto escuro. |
| Saturação e RGB | E-ink colorido | Oferece seis predefinições de saturação, um controle deslizante de saturação de 0%–300% e correção RGB de 0%–200% quando a plataforma oferece suporte. |
| Reduzir tremulação | Monitores compatíveis no macOS | Elimina a cintilação visível do dithering e ativa automaticamente nos monitores marcados como e-ink. |
| Night Shift e True Tone | Monitores afetados por mudanças na temperatura de cor | Exclui o monitor selecionado no macOS. Exige aprovação de administrador e a reconexão do monitor; o ajuste permanece após sair. |
| Reduzir transparência e movimento | Painéis de atualização lenta | Simplifica a aparência do sistema. O macOS usa um auxiliar confirmado uma única vez pelo usuário. |
| Curvas avançadas | Ajustes específicos para o painel | Ajusta o ponto de transição, Gamma, ponto preto e ponto branco com um gráfico em tempo real e cinco predefinições nomeadas. |

<p align="center">
  <img src="../../docs/en/text-contrast-editorial.png" alt="Ilustração do contraste do texto antes e depois" width="31%">
  <img src="../../docs/en/video-enhance-editorial.png" alt="Ilustração da melhoria de vídeo antes e depois" width="31%">
  <img src="../../docs/en/saturation-editorial.png" alt="Ilustração da saturação antes e depois" width="31%">
</p>

> Estas imagens ilustram os controles. Os resultados dependem do painel e do
> material de origem.

## Instalação

### macOS 14+, Apple silicon

1. Baixe o DMG do macOS 2.6 pelo link acima.
2. Abra-o e arraste **E-Ink Assistant** para **Aplicativos**.
3. Tente abrir o app uma vez. Se o macOS bloquear, abra **Ajustes do Sistema →
   Privacidade e Segurança** e selecione **Abrir Mesmo Assim**.

Este software é desenvolvido de forma independente e atualmente não está na
App Store. O macOS mostrará um aviso de que “não é possível verificar” na primeira
vez que você abri-lo. O código é totalmente aberto, então você pode analisá-lo
antes de decidir se deseja usá-lo.

Se **Abrir Mesmo Assim** não aparecer depois de mover o app para Aplicativos,
abra o Terminal e execute:

```
xattr -dr com.apple.quarantine "/Applications/E-Ink Assistant.app"
```

### Windows 7 SP1 ao Windows 11, x64

1. Baixe o instalador do Windows 1.2 pelo link acima.
2. Execute o instalador e aprove a solicitação de administrador.
3. Abra o E-Ink Assistant pelo menu Iniciar ou pela bandeja do sistema.

Consulte [WINDOWS.md](../../WINDOWS.md) para saber a disponibilidade exata dos
recursos conforme a versão do Windows, a GPU, o driver e a conexão do monitor.

## Como usar

<p align="center">
  <img src="../../docs/en/app-displays-v2-1.png" alt="Seleção de monitores no E-Ink Assistant v2.1" width="440">
</p>

1. Abra o app pela barra de menus do macOS ou pela bandeja do sistema do Windows.
2. Marque cada monitor e-ink P&B ou colorido que você deseja ajustar.
3. Primeiro, defina um contraste equilibrado no menu do próprio monitor.
4. Escolha Contraste do texto para leitura ou Melhoria de vídeo para mídia; não use ambos.
5. No e-ink colorido, ajuste Saturação e RGB quando a plataforma oferecer suporte.

**Os ajustes dos monitores são restaurados ao sair** e reaplicados ao iniciar.
Ative **Iniciar ao entrar** para inicialização automática.

## Configuração do monitor

Defina um contraste equilibrado no menu do próprio monitor antes de ajustar o app.
As predefinições incluídas foram ajustadas visualmente em um
**Bigme B251 Pro** (R2 FW V2.0) usando **modo Web, Gamma do hardware nível 3,
contraste 50, modo de restauração de cores desativado**. Um painel P&B ou outro
modelo colorido precisará de valores próprios. O modo Avançado oferece a curva
completa, e os ajustes são salvos separadamente para cada monitor.

Reduzir tremulação funciona somente em Apple Silicon e fica oculto onde não há suporte.

<details>
<summary>Auxiliar do macOS para Reduzir transparência e movimento</summary>

No primeiro uso, é solicitada a confirmação de **Adicionar Atalho** no app Atalhos
da Apple. O auxiliar incluído aceita somente os comandos exatos de Texto `on` e
`off`, não produz saída e não aparece na Folha de Compartilhamento, no Spotlight,
nas Ações Rápidas nem na interface da tela bloqueada. Ele pode ser executado
enquanto o Mac está bloqueado. O app não lista nem inspeciona seus outros atalhos.

O modo automático ativa os dois ajustes quando um monitor marcado como e-ink é
conectado e os desativa depois que o último monitor marcado é desconectado.
Sair do app também os desativa.

</details>

## Documentação do projeto

- [CHANGELOG.md](../../CHANGELOG.md): o que mudou em cada versão
- [TECHNICAL.md](../../TECHNICAL.md): implementação, medições e abordagens que
  *não* funcionam no macOS moderno
- [mac-saturation](https://github.com/kiteretsu903/mac-saturation): investigação
  do mecanismo de cores e uma CLI para exportar perfis

## Licença e créditos

MIT; consulte [LICENSE](../../LICENSE).

**Reduzir tremulação é baseado no [Stillcolor](https://github.com/aiaf/Stillcolor)
de Abdullah Arif** (MIT). O Stillcolor descobriu que o dithering do monitor pode
ser desativado pela propriedade `enableDither` do I/O Registry. Este projeto
reimplementa a ideia por monitor; o crédito pela descoberta pertence ao
Stillcolor. Obrigado.

Avisos completos em [THIRD-PARTY-NOTICES.md](../../THIRD-PARTY-NOTICES.md).
