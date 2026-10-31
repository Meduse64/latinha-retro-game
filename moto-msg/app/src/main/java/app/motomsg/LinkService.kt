package app.motomsg

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.Service
import android.content.Context
import android.content.Intent
import android.os.Build
import android.os.Handler
import android.os.IBinder
import android.os.Looper
import android.os.VibrationEffect
import android.os.Vibrator
import android.provider.Settings
import com.google.android.gms.nearby.Nearby
import com.google.android.gms.nearby.connection.AdvertisingOptions
import com.google.android.gms.nearby.connection.ConnectionInfo
import com.google.android.gms.nearby.connection.ConnectionLifecycleCallback
import com.google.android.gms.nearby.connection.ConnectionResolution
import com.google.android.gms.nearby.connection.ConnectionsClient
import com.google.android.gms.nearby.connection.DiscoveredEndpointInfo
import com.google.android.gms.nearby.connection.DiscoveryOptions
import com.google.android.gms.nearby.connection.EndpointDiscoveryCallback
import com.google.android.gms.nearby.connection.Payload
import com.google.android.gms.nearby.connection.PayloadCallback
import com.google.android.gms.nearby.connection.PayloadTransferUpdate
import com.google.android.gms.nearby.connection.Strategy

/** Estado compartilhado com a tela (tudo na thread principal). */
object Link {
    var status = "Parado"
    var onStatus: (() -> Unit)? = null
    var onMessage: ((Msg) -> Unit)? = null   // usado pela garupa para ver a resposta
    var send: ((Msg) -> Boolean)? = null
    var uiVisible = false   // MainActivity em primeiro plano

    fun update(s: String) { status = s; onStatus?.invoke() }
}

/**
 * Piloto: anuncia o código e mostra mensagens em tela cheia.
 * Garupa: procura o piloto com o mesmo código e envia mensagens.
 * Conexão direta Bluetooth/Wi-Fi via Nearby Connections: não precisa de internet.
 */
class LinkService : Service() {
    private lateinit var client: ConnectionsClient
    private lateinit var overlay: Overlay
    private val main = Handler(Looper.getMainLooper())
    private var pilot = true
    private var code = ""
    private var peer: String? = null

    override fun onBind(i: Intent?): IBinder? = null

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        if (intent?.action == ACTION_STOP) { stopSelf(); return START_NOT_STICKY }
        pilot = intent?.getBooleanExtra(EXTRA_PILOT, true) ?: true
        code = intent?.getStringExtra(EXTRA_CODE) ?: "0000"
        startForeground(1, notification())

        client = Nearby.getConnectionsClient(this)
        overlay = Overlay(this) { m, answer -> reply(m, answer) }
        Link.send = { sendMsg(it) }
        begin()
        return START_STICKY
    }

    override fun onDestroy() {
        if (::client.isInitialized) client.stopAllEndpoints()
        if (::client.isInitialized) { client.stopAdvertising(); client.stopDiscovery() }
        if (::overlay.isInitialized) overlay.dismiss()
        Link.send = null
        Link.update("Parado")
    }

    private fun begin() {
        peer = null
        if (pilot) {
            Link.update("Aguardando a garupa (código $code)…")
            client.startAdvertising(
                code, SERVICE_ID, lifecycle,
                AdvertisingOptions.Builder().setStrategy(Strategy.P2P_POINT_TO_POINT).build(),
            ).addOnFailureListener { retry("Erro ao anunciar: ${it.message}") }
        } else {
            Link.update("Procurando o piloto (código $code)…")
            client.startDiscovery(
                SERVICE_ID, discovery,
                DiscoveryOptions.Builder().setStrategy(Strategy.P2P_POINT_TO_POINT).build(),
            ).addOnFailureListener { retry("Erro ao procurar: ${it.message}") }
        }
    }

    private fun retry(why: String) {
        Link.update(why)
        main.postDelayed({ client.stopAdvertising(); client.stopDiscovery(); begin() }, 3000)
    }

    // Garupa: só se conecta ao piloto cujo nome (= código) bate.
    private val discovery = object : EndpointDiscoveryCallback() {
        override fun onEndpointFound(id: String, info: DiscoveredEndpointInfo) {
            if (info.endpointName == code && peer == null) {
                client.requestConnection("garupa", id, lifecycle)
            }
        }
        override fun onEndpointLost(id: String) {}
    }

    private val lifecycle = object : ConnectionLifecycleCallback() {
        override fun onConnectionInitiated(id: String, info: ConnectionInfo) {
            // O código já filtrou o par; aceita automaticamente (ninguém mexe no celular rodando).
            client.acceptConnection(id, payloads)
        }
        override fun onConnectionResult(id: String, r: ConnectionResolution) {
            if (r.status.isSuccess) {
                peer = id
                client.stopAdvertising(); client.stopDiscovery()
                Link.update("Conectado ✔")
            } else {
                retry("Falha na conexão, tentando de novo…")
            }
        }
        override fun onDisconnected(id: String) {
            peer = null
            retry("Desconectado, reconectando…")
        }
    }

    private val payloads = object : PayloadCallback() {
        override fun onPayloadReceived(id: String, p: Payload) {
            val m = runCatching { Msg.fromBytes(p.asBytes() ?: return) }.getOrNull() ?: return
            main.post { incoming(m) }
        }
        override fun onPayloadTransferUpdate(id: String, u: PayloadTransferUpdate) {}
    }

    private fun incoming(m: Msg) {
        buzz()
        if (pilot && m.kind == "msg") {
            if (Settings.canDrawOverlays(this)) overlay.show(m)
        } else if (Link.uiVisible && Link.onMessage != null) {
            Link.onMessage?.invoke(m)
        } else {
            notifyReply(m)
        }
    }

    // Garupa com o app em segundo plano / tela apagada: mostra a resposta numa notificação.
    private fun notifyReply(m: Msg) {
        val nm = getSystemService(NotificationManager::class.java)
        nm.createNotificationChannel(
            NotificationChannel("reply", "Respostas", NotificationManager.IMPORTANCE_HIGH),
        )
        val open = android.app.PendingIntent.getActivity(
            this, 1, Intent(this, MainActivity::class.java).addFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP),
            android.app.PendingIntent.FLAG_IMMUTABLE,
        )
        nm.notify(2, Notification.Builder(this, "reply")
            .setSmallIcon(android.R.drawable.stat_notify_chat)
            .setContentTitle("Resposta do piloto")
            .setContentText(m.text)
            .setStyle(Notification.BigTextStyle().bigText(m.text))
            .setContentIntent(open).setAutoCancel(true)
            .build())
    }

    private fun reply(orig: Msg, answer: String) {
        sendMsg(Msg("reply", orig.id, answer))
    }

    private fun sendMsg(m: Msg): Boolean {
        val to = peer ?: return false
        client.sendPayload(to, Payload.fromBytes(m.toBytes()))
        return true
    }

    private fun buzz() {
        val v = getSystemService(Context.VIBRATOR_SERVICE) as Vibrator
        v.vibrate(VibrationEffect.createWaveform(longArrayOf(0, 200, 100, 200), -1))
    }

    private fun notification(): Notification {
        val nm = getSystemService(NotificationManager::class.java)
        nm.createNotificationChannel(
            NotificationChannel("link", "Conexão", NotificationManager.IMPORTANCE_LOW),
        )
        val stop = android.app.PendingIntent.getService(
            this, 0, Intent(this, LinkService::class.java).setAction(ACTION_STOP),
            android.app.PendingIntent.FLAG_IMMUTABLE,
        )
        return Notification.Builder(this, "link")
            .setSmallIcon(android.R.drawable.stat_notify_chat)
            .setContentTitle("MotoMsg ligado (${if (pilot) "piloto" else "garupa"})")
            .addAction(Notification.Action.Builder(null, "Parar", stop).build())
            .build()
    }

    companion object {
        const val SERVICE_ID = "app.motomsg"
        const val EXTRA_PILOT = "pilot"
        const val EXTRA_CODE = "code"
        const val ACTION_STOP = "stop"
    }
}
