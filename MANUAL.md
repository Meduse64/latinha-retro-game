# Manual da Latinha Retro (CYD)

Um console portátil feito com a placa **CYD (ESP32-2432S028R)**: joga **Game Boy, Game Gear e Master System**, e mostra **Tempo, Ônibus e Agenda**. A tela fica deitada (320×240), com 8 botões físicos e toque.

Este manual é para quem **usa** o console. No fim há um capítulo curto para quem vai **compilar** o firmware. Como ligar os botões está em [BOTOES.md](BOTOES.md), e as decisões do projeto em [PORTE_CYD.md](PORTE_CYD.md).

> Os textos na tela não têm acento, porque a fonte da tela não os tem. Este manual tem.

---

## 1. O que você precisa

| Item | Observação |
|------|------------|
| CYD ESP32-2432S028R | Com a tela touch de 2,8" |
| 8 botões em um módulo PCF8574 | Cima, Baixo, Esquerda, Direita, A, B, Start, Select. Ligação no conector **CN1** (SDA no IO22, SCL no IO27), endereço `0x20`. Veja [BOTOES.md](BOTOES.md) |
| Cartão microSD | Formatado em **FAT32** |
| Rede Wi-Fi de 2,4 GHz | Só para Tempo, Ônibus, Agenda, acertar a hora e enviar jogos. Os jogos funcionam sem Wi-Fi |

### Pastas do cartão

```
/roms/gb     jogos de Game Boy (.gb) e de Game Boy Color com modo Game Boy (.gbc)
/roms/gbc    idem (as duas pastas são lidas)
/roms/gg     Game Gear (.gg)
/roms/sms    Master System (.sms)
/saves       salvamentos (.sav) e fotos do jogo (.state), criados pelo console
```

O console cria as pastas sozinho na primeira vez.

---

## 2. Tela inicial

Quatro blocos. Cada um tem o ícone à esquerda, um texto grande ao lado e uma linha de detalhe embaixo (se a linha não couber, ela desfila).

| Bloco | Texto grande | Embaixo |
|-------|--------------|---------|
| **Retro** | `Retro` | `GB  GG  SMS` |
| **Tempo** | a temperatura de agora (o ícone muda com o clima) | o clima, por exemplo `Poucas nuvens` |
| **Onibus** | a linha que sai primeiro (`Metro` ou `S.All`) | quanto falta, por exemplo `em 10 min` |
| **Agenda** | a data do próximo evento, por exemplo `10 Nov` | a hora e o título, por exemplo `14:30 Fisio` |

No canto de cima fica a hora. Ela só aparece depois que o console acertou o relógio pela internet (a primeira vez que você abrir Tempo, Ônibus ou Agenda).

### Como usar

| Ação | Toque | Botões |
|------|-------|--------|
| Abrir um bloco | Toque no bloco | Setas escolhem, **A** abre |
| **Calibrar o toque** | Toque no rodapé | **Select** |
| **Ligar os olhos** (protetor de tela) | Toque na palavra **Latinha** no alto | — |

---

## 3. Controles em todo o console

| Ação | Toque | Botões |
|------|-------|--------|
| Voltar para a tela inicial | Toque no título, no alto à esquerda | **B** ou **Start** (na lista do Retro, só o **B**) |
| Rolar uma lista | Setas ▲ ▼ na tela | **Cima** e **Baixo** |
| Atualizar os dados (Tempo, Agenda) | Toque no rodapé | **A** |
| **Abrir a pausa dentro de um jogo** | Toque em **qualquer lugar** da tela | **Start + Select** juntos |

O toque é resistivo e não arrasta bem: por isso as listas rolam por setas, não por deslize.

---

## 4. Retro (os jogos)

### A lista

Mostra 4 jogos por página, com um selo por sistema: **GB**, **GG** ou **SMS**. Na barra de baixo:

| Parte | O que faz |
|-------|-----------|
| **OPCOES** (esquerda) | Abre as opções (capítulo 5) |
| **< 1 / 2 >** (meio) | Troca de página. Toque na metade esquerda ou direita, ou use **Esquerda** e **Direita** |
| **WI-FI** (direita) | Envia jogos pelo Wi-Fi (capítulo 6) |

Para abrir um jogo: toque nele (o primeiro toque seleciona, o segundo abre) ou escolha com **Cima/Baixo** e aperte **A**. **B** volta para a tela inicial.

A primeira vez que você abre um jogo de **Game Gear ou Master System**, o console copia a ROM do cartão para a memória interna. Isso leva alguns segundos; nas próximas vezes é imediato (enquanto você não abrir outro jogo desses).

### Os botões dentro do jogo

| Sistema | Botões do console |
|---------|-------------------|
| Game Boy | Setas, **A**, **B**, **Start**, **Select**, como no original |
| Game Gear | Setas, **B** = botão 1, **A** = botão 2, **Start** = Start |
| Master System | Setas, **B** = botão 1, **A** = botão 2, **Start** = Pause |

**Start + Select** (ou um toque na tela) abre a pausa, por isso nos jogos eles não valem como botões do jogo.

### A pausa

| Item | O que faz |
|------|-----------|
| **Continuar** | Volta ao jogo (o **B** também) |
| **Salvar foto** | Guarda o ponto exato do jogo, em `/saves/<jogo>.state` |
| **Carregar foto** | Volta ao ponto guardado |
| **Opcoes** | Tamanho, paleta, pular quadros e brilho, sem sair do jogo |
| **Sair** | Volta para a lista de jogos |

Os jogos com bateria (por exemplo o Zelda) guardam o progresso no próprio cartão, em `/saves/<jogo>.sav`.

### Jogos de Game Boy Color

O console **não tem cor de Game Boy Color**. Os jogos `.gbc` que também funcionam no Game Boy comum (a maioria) aparecem na lista com o selo **GB** e rodam **sem cor**, com a paleta que você escolher. Os jogos que são **só de GBC** (o Super Mario Bros. Deluxe, por exemplo) não rodam e **não aparecem** na lista.

---

## 5. Opções

Pela lista (botão **OPCOES**) ou pela pausa.

| Opção | Valores |
|-------|---------|
| **Tamanho** | **Normal** (1:1), **Ajustado** (×1,5), **Cheia** (×1,67, sem distorcer) e **Esticada** (320×240, distorce). O que sobra da tela fica preto |
| **Paleta** | As cores do Game Boy (vale para os jogos de Game Boy) |
| **Pular quadros** | 0 a 4. O valor **1** é um bom ponto para os jogos rodarem fluidos. Em Game Gear e Master System o jogo continua certo, mesmo pulando quadros |
| **Brilho** | 8 níveis |
| **Enviar jogos pelo Wi-Fi** | Só nas opções da lista |

Toque na metade esquerda ou direita da linha para diminuir ou aumentar (ou **Esquerda/Direita**). **PRONTO** (ou **B**) salva e sai. As opções ficam guardadas.

---

## 6. Enviar jogos pelo Wi-Fi

Não precisa tirar o cartão.

1. Na lista do Retro, toque em **WI-FI** (ou em Opções, "Enviar jogos pelo Wi-Fi").
2. O console liga o Wi-Fi e mostra um endereço grande, por exemplo `http://192.168.0.147`.
3. No computador ou no celular, **na mesma rede**, abra esse endereço **escrevendo o `http://` na frente**. Sem ele, alguns navegadores tentam `https` e dão "conexão recusada".
4. Arraste as ROMs para a página, ou toque para escolhê-las. Servem `.gb`, `.gbc`, `.gg` e `.sms`; cada uma vai para a pasta certa do cartão. Um arquivo com o mesmo nome substitui o antigo.
5. A página também lista os jogos do cartão e tem o botão **Apagar** (com confirmação).
6. Para sair, **B**, **Start** ou toque no título. Depois de 10 minutos sem uso o Wi-Fi desliga sozinho.

Uma ROM de 1 MB leva uns 6 segundos. **A página não tem senha**: use só na rede de casa.

---

## 7. Tempo, Ônibus e Agenda

Eles pedem Wi-Fi só na hora de buscar os dados e depois desligam. Os dados do Wi-Fi ficam no `secrets.h` (capítulo 10).

### Tempo

Previsão do Rio de Janeiro pelo Open-Meteo (sem cadastro). Três abas, trocadas com o toque ou com **Esquerda/Direita**:

| Aba | Mostra |
|-----|--------|
| **AGORA** | Temperatura, sensação, umidade, vento e rajada (com a direção), chance de chuva hoje, nascer e pôr do sol |
| **HORAS** | As próximas 24 horas: hora, temperatura, chance de chuva e vento (setas ▲ ▼ rolam) |
| **7 DIAS** | Máxima, mínima, chuva e vento de cada dia |

Atualiza sozinho quando passam 30 minutos; **A** atualiza na hora. Para outra cidade, mude `CITY_LAT` e `CITY_LON` no começo de `app_tempo.cpp`.

### Ônibus

As próximas saídas da parada Pontal: **Metro** e **S.Allende**, com os horários de dias úteis, sábado e domingo. Toque numa linha (ou **Cima/Baixo**) para ver as próximas 6 saídas dela. O tempo que falta fica **verde** (mais de 15 min), **âmbar** (até 15) ou **vermelho** (até 5). Os horários ficam fixos no código, em `app_onibus.cpp`: se a tabela mudar, é lá que se edita.

### Agenda

Os próximos eventos do **Google Calendar**, pelo "endereço secreto no formato iCal". Mostra até 8 eventos (5 por tela), com a data e a hora; os repetidos (por semana, mês ou ano) aparecem na data certa. Atualiza a cada 15 minutos; **A** atualiza na hora. Um evento cancelado só em uma das datas de uma série ainda aparece.

Para pegar o endereço: no Google Calendar, abra as configurações da sua agenda e copie o **endereço secreto no formato iCal**.

---

## 8. Protetor de tela e sono

| Quando | O que acontece |
|--------|----------------|
| **5 min sem usar**, nos menus e apps | Aparecem uns olhos que olham em volta e piscam, com a tela mais fraca |
| **10 min sem usar** | Os olhos fecham, a tela apaga e a placa entra em **deep sleep** (quase sem consumo) |
| **Toque na tela** | Sai dos olhos ou acorda do sono. O toque que acorda não vira clique. Ao acordar do sono a placa reinicia e vai direto para a tela inicial |
| **Toque em "Latinha"** na tela inicial | Os olhos ligam na hora e ficam até alguém tocar. Nesse caso a placa **não dorme** |

Dentro de um jogo o protetor **não** entra: o jogo pode estar rodando sem você apertar nada. Na tela de envio de jogos ele também não entra.

---

## 9. Quando algo não funciona

| Sintoma | O que fazer |
|---------|-------------|
| O toque cai fora do lugar | Na tela inicial, aperte **Select** (ou toque no rodapé) e toque nas 5 cruzes |
| A hora mostra `--:--` | Abra Tempo, Ônibus ou Agenda uma vez, com Wi-Fi ligado, para acertar o relógio |
| "Sem Wi-Fi" | Confira o nome e a senha no `secrets.h` e se a rede é de 2,4 GHz |
| A página de envio dá "conexão recusada" | Escreva `http://` antes do número |
| A página não abre | O computador ou celular precisa estar na mesma rede que a CYD |
| "Nenhum jogo no cartao" | Veja se o cartão é FAT32 e se as ROMs estão nas pastas de `/roms`. Envie pelo botão **WI-FI** |
| Um jogo `.gbc` não aparece | Ele é só de Game Boy Color (capítulo 4). Esse não roda |
| O jogo de Game Gear ou Master System demora para abrir | É a cópia da ROM para a memória interna, só da primeira vez |
| O jogo está lento | Em Opções, **Pular quadros** em 1 |
| "Erro no cartao SD!" | Cartão ausente ou fora do FAT32. Coloque um cartão FAT32 e reinicie |
| A tela apagou e não volta | Ela dormiu: toque na tela |
| Os olhos aparecem parados | Faltou memória para animá-los. Toque na tela e tente de novo |

Medidas no aparelho: Game Boy ~58 FPS; Game Gear e Master System 50 a 61 FPS.

---

## 10. Para quem compila o firmware

1. **Arduino IDE** com o núcleo **esp32 3.3.11**, e as bibliotecas **TFT_eSPI 2.5.43** e **ArduinoJson 7**. Não precisa editar o `User_Setup` do TFT_eSPI: o `build_opt.h` do firmware já traz a configuração da tela, do toque e do cartão.
2. **Placa:** ESP32 Dev Module, Flash Mode QIO, 80 MHz, e **Partition Scheme: "No OTA (2MB APP/2MB SPIFFS)"**.
3. Copie `src/secrets.example.h` para `src/secrets.h` e preencha `WIFI_SSID`, `WIFI_PASS` e `GOOGLE_ICS_URL`. Esse arquivo **não vai para o git**.
4. Para ter **Game Gear e Master System**, rode `scripts\copiar_smsplus.ps1`: ele copia o SMS Plus do projeto LatinhaColor para `src/smsplus/` (pasta que o git ignora) e junta o `cyd_collide.c`. Sem essa pasta o firmware compila, só sem esses dois sistemas.
5. Abra `firmware/cydboy_fw/cydboy_fw.ino`, compile e grave. A primeira compilação demora uns 10 minutos.

Por linha de comando (com o arduino-cli que vem com o Arduino IDE), o `fqbn` é `esp32:esp32:esp32:FlashMode=qio,FlashFreq=80,PartitionScheme=no_ota`.

**Licenças:** o Peanut-GB e o CYDboy são MIT. O SMS Plus é GPL v2 e o Z80 é "só uso não comercial", por isso não estão no repositório; o `cyd_collide.c`, derivado do SMS Plus, é GPL v2 ou posterior. Detalhes no capítulo 8 de [PORTE_CYD.md](PORTE_CYD.md).
