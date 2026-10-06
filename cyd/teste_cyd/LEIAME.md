# Teste de hardware do CYD (fase 1)

Um sketch da Arduino IDE para conferir, antes de qualquer emulador, se o CYD em pé, a tela ampliada ×1,5, os botões, o cartão SD e a memória estão funcionando. Já foi **compilado e gravado num CYD**: arduino-cli 1.5.1, núcleo ESP32 3.3.11, placa ESP32 Dev Module, Adafruit GFX 1.12.6 e Adafruit ILI9341 1.6.3. Ocupa 364 KB de programa e 25 KB de RAM. Depois de gravado ele ligou e imprimiu no Monitor Serial, mas o que aparece na tela, o toque, o PCF8574 e o cartão SD **ainda não foram conferidos**: quem escreveu o código não vê a tela da placa.

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
| Toque | Já está na placa (XPT2046 nos IO25, 32, 33, 36 e 39). Não há nada a ligar. Um toque em qualquer lugar da tela também acende a tecla MENU (`USE_TOUCH_AS_HOME` em `config_cyd.h`) |

Se o PCF8574 não estiver no endereço 0x20, o sketch procura sozinho entre 0x20 a 0x27 e 0x38 a 0x3F e mostra o endereço achado.

## O que deve aparecer

- Em cima (240×192): a barra azul **TESTE CYD**, o tempo de envio de cada quadro, as faixas de cor e as 9 teclas. Cada uma fica verde quando apertada e conta os cliques.
- Embaixo: três quadrados **vermelho, verde e azul**, que são a conferência de cores.
- A linha do PCF8574 (verde com o byte lido, ou vermelha se não achou), a do ADKeyboard (tensão em mV), o cartão SD (tamanho e quantos jogos em `roms/gb`, `gbc`, `gg`, `sms`) e a memória livre.
- A última linha é o **toque**. Sem dedo, mostra "Toque a tela" e quantos toques já houve. Com o dedo, mostra a posição na tela e, entre parênteses, os valores brutos do XPT2046. Uma **cruz amarela** segue o dedo, tanto na parte de cima quanto na faixa de baixo. Qualquer toque acende o quadrado **MENU**, que é o comportamento planejado para o menu de pausa.

## Se algo estiver errado

| Sintoma | O que fazer |
|---------|-------------|
| Tela branca ou apagada | Confirme a placa (ESP32 Dev Module) e que a luz de fundo está no IO21 |
| Cores trocadas ou invertidas | `TFT_INVERT` em `config_cyd.h` (`true`/`false`) |
| Ruído ou linhas na imagem | `TFT_HZ` para `27000000UL` |
| "SD: nao montou" | O cartão tem de estar em **FAT32** (exFAT não funciona) e bem encaixado |
| "PCF8574: nao achado" | Olhe a linha cinza logo abaixo, `I2C SDA:... SCL:...`, que se atualiza a cada 0,5 s e deixa mudar os fios com a placa ligada. Os dois têm de mostrar `alto`. `GND` num deles quer dizer fio ligado ao pino errado do módulo (o cabo do CN1 pode vir com a ordem invertida em relação aos rótulos do módulo: confira SCL, SDA, GND e VCC). `solto` quer dizer sem fio ou módulo sem energia. Se aparecer "SDA e SCL TROCADOS", troque esses dois fios. Os mesmos dados saem no Monitor Serial |
| Imagem de lado, espelhada ou com vermelho e azul trocados | Mude `TFT_MADCTL` em `config_cyd.h`. Neste CYD, o valor da biblioteca (0x48) mostrou a imagem de lado e com cores trocadas. Os valores em pé prováveis são 0x20 e 0xE0 (diferem em 180 graus). Se ainda sair espelhado, tente 0x60, 0xA0, 0x00, 0x40, 0x80 ou 0xC0 |
| Tecla do ADKeyboard errada | Veja o valor de "ADK mV" e ajuste os `KB_SW*_MAX` em `config_cyd.h` |
| A cruz anda ao contrário na horizontal | `TOUCH_INV_X` em `config_cyd.h` para `1` |
| A cruz anda ao contrário na vertical | `TOUCH_INV_Y` para `1` |
| Ao deslizar na horizontal a cruz anda na vertical (e o contrário) | `TOUCH_SWAP_XY` para `0` (ou `1`, se estiver em `0`) |
| A cruz não chega ao canto, ou passa dele | Toque cada canto e anote os valores brutos (entre parênteses). Ajuste `TOUCH_RAWX_MIN/MAX` e `TOUCH_RAWY_MIN/MAX` |
| O MENU acende sozinho, sem dedo | O IRQ do toque (IO36) pode estar lendo ruído. Ponha `USE_TOUCH_AS_HOME` em `0` para separar o teste, e mande as linhas `toque raw` do Monitor Serial |

## O que me mandar depois

Uma foto da tela, os valores brutos do toque nos quatro cantos (cada toque imprime uma linha `toque raw X,Y -> tela X,Y` no Monitor Serial) e as primeiras linhas do Monitor Serial (a que começa com "Memoria livre"). Dessa linha saem a memória livre real do CYD, o endereço do PCF8574 e se o SD montou. O tempo de envio por quadro (em ms, no canto da barra) diz quanto o SPI aguenta, e define como os emuladores vão desenhar.
