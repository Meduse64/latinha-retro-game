# MotoMsg

App Android (um só APK, dois papéis) para falar entre piloto e garupa sem tirar os olhos do GPS.

- **Garupa**: toca numa mensagem pronta (ou escreve) e envia. Pode pedir resposta (Sim/Não, OK…).
- **Piloto**: a mensagem aparece em **tela cheia, letra gigante, por cima do GPS**. Tocar no texto fecha e volta ao GPS. Se a garupa pediu resposta, tocar num botão responde (e fecha).
- Conexão direta Bluetooth/Wi-Fi (Google Nearby Connections): **não precisa de internet**. Os dois celulares usam o mesmo código de 4 números.

## Uso
1. Instale nos dois celulares; os dois escolhem o mesmo código.
2. Piloto: permitir "Sobrepor a outros apps", tocar em **Ligar**. Use "Testar tela grande" para ver o efeito.
3. Garupa: tocar em **Conectar**. Quando aparecer "Conectado ✔", enviar.

## Compilar
Abra a pasta `moto-msg/` no Android Studio (Sync + Run), ou `gradle assembleDebug` com o Android SDK instalado.
Requer Android 8.0+ e Google Play Services nos dois celulares.

## Estado
Primeira versão, escrita sem SDK disponível: **ainda não compilada nem testada em aparelho**.
Ideias: ler a mensagem em voz alta (TTS) pelo fone do capacete, mensagens prontas editáveis.
