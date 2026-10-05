# Porte do app Retro (LatinhaColor) para o CYD

Este documento parte do arquivo do app Retro do firmware anterior (LatinhaColor), que você colou na conversa. Eu só vi **esse arquivo**: não tenho o `gb_core.c`, o `src/smsplus`, o restante do firmware (tela, botões, menu) nem o script `gravar_jogos.ps1`. Tudo abaixo é leitura do código colado e **não foi compilado nem testado**.

---

## 1. O que o app Retro já faz

- **Game Boy** com o Peanut-GB (MIT), **Game Gear** e **Master System** com o SMS Plus (GPL v2, versão adaptada do esp_8_bit).
- Os jogos ficam na partição `spiffs` (esquema "No OTA"), gravados pelo `gravar_jogos.ps1`, com um índice `GBR3` de até 16 jogos. A partição é mapeada como memória (`esp_partition_mmap`), então o SMS Plus lê a ROM direto.
- **Save de bateria** (`BAT3`) e **foto / save state** (`STA3`) por jogo, gravados na própria partição. O cabeçalho é gravado por último, então um save interrompido não vale.
- Menu de pausa com o botão amarelo: Continuar, Salvar foto, Carregar foto, Sair.
- Cada linha emulada vai direto para o buffer da tela (`gbw_lcd_line`, `latinha_sms_line`), e o envio para o LCD roda numa tarefa no núcleo 0 enquanto o emulador faz o quadro seguinte.
- Sem som (`emu_system_init(0)`).

Isso é a opção C da `PROPOSTA.md` (firmware próprio), mas com a parte difícil já escrita.

---

## 2. O que muda no CYD

| Item | LatinhaColor (pelo código) | CYD |
|------|----------------------------|-----|
| Tela | 160×128 (`SW` = 160) | ILI9341 **320×240** |
| Buffer | `gfx` de 16 bits, 160×128 (cerca de 40 KB) | 320×240 em 16 bits são 150 KB: evitar. Usar um buffer do tamanho do jogo (GB/GG 160×144 = 46 KB; SMS 256×192 = 98 KB) |
| Escala | GB e GG: descarta 1 linha a cada 9 (144→128). SMS: 256×192 → 160×128 (descarta linhas e colunas) | GB e GG em **160×144, 1:1** (ou 1,5× = 240×216). SMS em **256×192, 1:1**. Some o código de descarte (`gl % 9`, `line % 3`, `smsX`) |
| Taxa de quadros | SMS desenha 1 quadro sim, 1 não | Medir. A 40 MHz de SPI, 98 KB levam cerca de 20 ms e 46 KB cerca de 9 ms (conta minha, sem testar) |
| Botões | Joystick 5D + SW1–SW4 + amarelo | PCF8574 no CN1 + botão BOOT (secção 3) |
| Rede | Telegram (`tgIdle`, `netPause`, tarefa no núcleo 0) | Some: sem Wi-Fi, sobra mais RAM |
| ROMs | Partição `spiffs` de cerca de 2 MB (esquema "No OTA" de 4 MB, a confirmar) | Mesma coisa para começar. Depois, microSD (secção 5) |
| Som | Nenhum | Alto-falante no GPIO26, opcional |

---

## 3. Botões: dá para usar o que você já tem

O app usa 9 teclas: cima, baixo, esquerda, direita, `KEY_OK`, `KEY_BACK`, `KEY_A`, `KEY_B` e `KEY_HOME`. O PCF8574 tem 8 pinos, e o amarelo (`KEY_HOME`) vai para o botão BOOT do CYD (GPIO0, já na placa). **Não precisa comprar o PCF8575.**

| Pino do PCF8574 | Tecla do app | Peça | Game Boy | Game Gear / Master System |
|-----------------|--------------|------|----------|---------------------------|
| P0 | `KEY_UP` | módulo 5D: UP | cima | cima |
| P1 | `KEY_DOWN` | módulo 5D: DWN | baixo | baixo |
| P2 | `KEY_LEFT` | módulo 5D: LFT | esquerda | esquerda |
| P3 | `KEY_RIGHT` | módulo 5D: RHT | direita | direita |
| P4 | `KEY_OK` | botão do kit nº 1 | A | botão 2 |
| P5 | `KEY_BACK` | botão do kit nº 2 | B | botão 1 |
| P6 | `KEY_B` | módulo 5D: RST | Start | Start (GG) / Pause (SMS) |
| P7 | `KEY_A` | módulo 5D: SET | Select | (sem uso) |
| GPIO0 (BOOT) | `KEY_HOME` | botão da placa | menu de pausa | menu de pausa |

Ligações do PCF8574 no CN1: SDA no IO22, SCL no IO27, VCC no 3V3, GND no GND. O COM do módulo 5D e o outro lado de cada botão vão no GND. O endereço depende dos jumpers A0/A1/A2. O sketch de teste está na secção 5.3 do `BOTOES.md`.

Dois detalhes:
- O GPIO0 é um pino de boot: o botão precisa estar solto na hora de ligar ou de resetar. Depois que o programa roda, pode ser usado como entrada normal.
- O MID (centro) do módulo 5D fica sem uso.

---

## 4. O que eu preciso de você

Só tenho o arquivo do app. Para montar o projeto do CYD preciso dos outros arquivos do firmware anterior:

1. `gb_core.c` e o `peanut_gb.h`, com os wrappers `gbw_*`.
2. A pasta `src/smsplus/` inteira, incluindo as alterações que você fez (`latinha_sms_line`, `latinha_sms_piece`, `latinha_alloc_sram`, `sms_frame`, `emu_system_init`).
3. A parte do firmware que define o que o app usa: `gfx` (e `getBuffer()`), `flush()`, `SW`/`SH`, `rgb()`, as cores `C_*`, `txt`/`txtC`/`txtR`/`statusBar`/`hint`, `keys[]`, `Key`, `BtnEvt`, `isPress`, `appOwnsScreen`.
4. O esquema de partições (arquivo CSV ou a opção da placa) e o `gravar_jogos.ps1`.
5. O `platformio.ini` ou a configuração do Arduino (placa, bibliotecas e versões).

**Como enviar:** copie tudo para uma pasta deste repositório (por exemplo `latinha-color/`) e dê push na branch `claude/retro-game-proposal-07kbyg`. **Sem as ROMs.** Deixei um `.gitignore` que bloqueia `roms/` e as extensões `.gb`, `.gbc`, `.gg`, `.sms`, `.nes`, `.sfc`, `.smc` e `.gba`, porque o repositório é público. Se o firmware anterior estiver em outro repositório do GitHub, ele não está liberado para esta sessão: dá para liberar nas configurações do app do Claude no GitHub.

---

## 5. Plano

| Fase | Entrega | Critério de sucesso |
|------|---------|---------------------|
| 1 | Esqueleto para o CYD: tela ILI9341, leitura dos botões (PCF8574 + BOOT), sem emuladores | Uma tela de teste mostra a tecla apertada |
| 2 | Game Boy | Um jogo roda com os botões, em velocidade correta |
| 3 | Game Gear e Master System | Os dois rodam, com a imagem 1:1 |
| 4 | Saves e fotos | Save de bateria e foto funcionando como no LatinhaColor |
| 5 | Extras | Som no GPIO26, ROMs no microSD (GB pode ler por callback; o SMS Plus precisa da ROM em memória, então a ideia é copiar a ROM do cartão para a partição ao escolher o jogo) |

O NES fica de fora por enquanto: o Anemoia-ESP32 é GPLv3 e o SMS Plus é GPL v2, e misturar os dois pode dar problema de licença. Se quiser NES, o caminho é o firmware do Anemoia separado, como descrito no `BOTOES.md`.

---

## 6. Licenças

O repositório é público. O SMS Plus é **GPL v2**: o firmware que o inclui, se for distribuído ou publicado, precisa seguir a GPL v2 (e manter os avisos de copyright). O Peanut-GB é MIT. Quando o código entrar aqui, adiciono um `LICENSE` e os avisos no lugar certo.
