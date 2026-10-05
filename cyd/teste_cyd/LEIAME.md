# Teste de hardware do CYD (fase 1)

Um sketch da Arduino IDE para conferir, antes de qualquer emulador, se o CYD em pé, a tela ampliada ×1,5, os botões, o cartão SD e a memória estão funcionando. **Não foi compilado para o ESP32** (o ambiente onde foi escrito não tem a toolchain). Foi compilado só no computador, com cabeçalhos simulados, e as assinaturas da Adafruit foram conferidas nos cabeçalhos reais.

## Instalar

1. Arduino IDE com o suporte a ESP32 (o mesmo do LatinhaColor, versão 3.x).
2. Em *Ferramentas, Gerenciar Bibliotecas*: **Adafruit GFX Library** e **Adafruit ILI9341** (ela instala a Adafruit BusIO sozinha).
3. Placa: **ESP32 Dev Module**. Velocidade de upload 460800 (se falhar, 115200).
4. Abra `teste_cyd/teste_cyd.ino` (a pasta tem de se chamar `teste_cyd`), conecte o CYD por USB e carregue. Se aparecer "Connecting..." e travar, segure o botão **BOOT** da placa até começar a gravar.
5. Abra o Monitor Serial em **115200**.

## Ligações (só o que você tiver)

| Peça | Ligação no CYD |
|------|----------------|
| PCF8574 | VCC no **3V3**, GND no **GND**, SDA no **IO22**, SCL no **IO27** (conector CN1). Confirme a ordem dos pinos pelas letras impressas na placa |
| Botões nos pinos P0 a P7 do PCF8574 | P0 cima, P1 baixo, P2 esquerda, P3 direita, P4 OK, P5 VOLTAR, P6 `B`, P7 `A` (cada botão entre o pino e o GND). Com o módulo 5D: UP, DWN, LFT e RHT nos quatro primeiros, RST em P6 e SET em P7 |
| ADKeyboard (opcional) | Sinal no **IO35** (conector P3), alimentação no 3V3 do CN1, GND comum. Ligue `USE_ADKEYBOARD` em `config_cyd.h` |
| Botão BOOT | Já está na placa e vira o botão MENU |

Se o PCF8574 não estiver no endereço 0x20, o sketch procura sozinho entre 0x20 a 0x27 e 0x38 a 0x3F e mostra o endereço achado.

## O que deve aparecer

- Em cima (240×192): a barra azul **TESTE CYD**, o tempo de envio de cada quadro, as faixas de cor e as 9 teclas. Cada uma fica verde quando apertada e conta os cliques.
- Embaixo: três quadrados **vermelho, verde e azul**, que são a conferência de cores.
- A linha do PCF8574 (verde com o byte lido, ou vermelha se não achou), a do ADKeyboard (tensão em mV), o cartão SD (tamanho e quantos jogos em `roms/gb`, `gbc`, `gg`, `sms`) e a memória livre.

## Se algo estiver errado

| Sintoma | O que fazer |
|---------|-------------|
| Tela branca ou apagada | Confirme a placa (ESP32 Dev Module) e que a luz de fundo está no IO21 |
| Cores trocadas ou invertidas | `TFT_INVERT` em `config_cyd.h` (`true`/`false`) |
| Ruído ou linhas na imagem | `TFT_HZ` para `27000000UL` |
| "SD: nao montou" | O cartão tem de estar em **FAT32** (exFAT não funciona) e bem encaixado |
| "PCF8574: nao achado" | Ligações de SDA/SCL, alimentação, e se os pinos de endereço A0/A1/A2 estão ligados |
| Tecla do ADKeyboard errada | Veja o valor de "ADK mV" e ajuste os `KB_SW*_MAX` em `config_cyd.h` |

## O que me mandar depois

Uma foto da tela e as primeiras linhas do Monitor Serial (a que começa com "Memoria livre"). Dessa linha saem a memória livre real do CYD, o endereço do PCF8574 e se o SD montou. O tempo de envio por quadro (em ms, no canto da barra) diz quanto o SPI aguenta, e define como os emuladores vão desenhar.
