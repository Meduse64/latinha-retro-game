package app.motomsg

import android.Manifest
import android.app.Activity
import android.content.Intent
import android.content.SharedPreferences
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.provider.Settings
import android.view.Gravity
import android.view.View
import android.widget.Button
import android.widget.EditText
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.TextView
import android.widget.Toast

class MainActivity : Activity() {
    private lateinit var prefs: SharedPreferences
    private lateinit var root: LinearLayout
    private lateinit var statusView: TextView
    private val dp get() = resources.displayMetrics.density

    // Mensagens prontas da garupa: (texto, respostas possíveis). Sem respostas = só aviso.
    private val presets = listOf(
        "Pare, por favor" to emptyList(),
        "Podemos parar?" to listOf("Sim", "Não"),
        "Quero ir ao banheiro" to listOf("OK", "Em 10 min"),
        "Está tudo bem?" to listOf("Tudo bem", "Calma!"),
        "Com fome?" to listOf("Sim", "Não"),
        "Com frio / com calor" to emptyList(),
        "Mais devagar, por favor" to emptyList(),
        "Pode seguir" to emptyList(),
    )

    override fun onCreate(b: Bundle?) {
        super.onCreate(b)
        prefs = getSharedPreferences("p", MODE_PRIVATE)
        root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(pad(16), pad(40), pad(16), pad(16))
        }
        setContentView(ScrollView(this).apply { addView(root) })
        statusView = text("", 18f)
        Link.onStatus = { runOnUiThread { statusView.text = Link.status } }
        Link.onMessage = { m -> runOnUiThread { showAnswer(m) } }
        render()
    }

    override fun onDestroy() { Link.onStatus = null; Link.onMessage = null; super.onDestroy() }

    private fun pad(v: Int) = (v * dp).toInt()

    private fun text(s: String, size: Float) = TextView(this).apply {
        text = s; textSize = size; setPadding(0, pad(8), 0, pad(8)); root.addView(this)
    }

    private fun button(label: String, onClick: () -> Unit) = Button(this).apply {
        text = label; textSize = 20f; isAllCaps = false
        setOnClickListener { onClick() }
        root.addView(this, LinearLayout.LayoutParams(-1, pad(72)).apply { topMargin = pad(8) })
    }

    private fun render() {
        root.removeAllViews()
        when (prefs.getString("role", null)) {
            "pilot" -> pilotScreen()
            "pillion" -> pillionScreen()
            else -> chooseScreen()
        }
    }

    private fun chooseScreen() {
        text("MotoMsg", 32f)
        text("Código do par (os dois celulares usam o mesmo, ex.: 4 números):", 16f)
        val code = EditText(this).apply {
            setText(prefs.getString("code", "1234")); textSize = 28f
            inputType = android.text.InputType.TYPE_CLASS_NUMBER
            root.addView(this)
        }
        fun pick(role: String) {
            prefs.edit().putString("role", role).putString("code", code.text.toString().ifBlank { "1234" }).apply()
            render()
        }
        button("Sou o PILOTO (celular com o GPS)") { pick("pilot") }
        button("Sou a GARUPA (envia mensagens)") { pick("pillion") }
    }

    // ---------- Piloto ----------
    private fun pilotScreen() {
        text("Piloto — código ${prefs.getString("code", "")}", 24f)
        root.addView(statusView); statusView.text = Link.status
        if (!Settings.canDrawOverlays(this)) {
            text("Falta permitir \"Sobrepor a outros apps\" para a mensagem aparecer por cima do GPS.", 16f)
            button("Permitir sobreposição") {
                startActivity(Intent(Settings.ACTION_MANAGE_OVERLAY_PERMISSION, Uri.parse("package:$packageName")))
            }
        }
        button("Ligar") { ensurePermissions { startLink(true) } }
        button("Testar tela grande") {
            if (!Settings.canDrawOverlays(this)) toast("Permita a sobreposição primeiro")
            else Overlay(this) { _, a -> toast("Resposta: $a") }
                .show(Msg("msg", "t", "Podemos parar?", listOf("Sim", "Não")))
        }
        button("Desligar") { stopService(Intent(this, LinkService::class.java)) }
        button("Trocar de papel") { prefs.edit().remove("role").apply(); render() }
    }

    // ---------- Garupa ----------
    private fun pillionScreen() {
        text("Garupa — código ${prefs.getString("code", "")}", 24f)
        root.addView(statusView); statusView.text = Link.status
        button("Conectar") { ensurePermissions { startLink(false) } }
        for ((t, opts) in presets) {
            button(if (opts.isEmpty()) t else "$t  ↩") { send(t, opts) }
        }
        val free = EditText(this).apply { hint = "Escrever mensagem…"; textSize = 20f; root.addView(this) }
        button("Enviar") { free.text.toString().trim().takeIf { it.isNotEmpty() }?.let { send(it, emptyList()); free.setText("") } }
        button("Enviar e pedir Sim/Não") { free.text.toString().trim().takeIf { it.isNotEmpty() }?.let { send(it, listOf("Sim", "Não")); free.setText("") } }
        button("Trocar de papel") {
            stopService(Intent(this, LinkService::class.java)); prefs.edit().remove("role").apply(); render()
        }
    }

    private fun send(t: String, opts: List<String>) {
        val ok = Link.send?.invoke(Msg("msg", System.currentTimeMillis().toString(), t, opts)) ?: false
        toast(if (ok) "Enviado" else "Não conectado")
    }

    private fun showAnswer(m: Msg) {
        android.app.AlertDialog.Builder(this)
            .setMessage(m.text)
            .setPositiveButton("OK", null)
            .show().findViewById<TextView>(android.R.id.message)?.apply { textSize = 48f; gravity = Gravity.CENTER }
    }

    // ---------- Comum ----------
    private fun startLink(pilot: Boolean) {
        val i = Intent(this, LinkService::class.java)
            .putExtra(LinkService.EXTRA_PILOT, pilot)
            .putExtra(LinkService.EXTRA_CODE, prefs.getString("code", "1234"))
        startForegroundService(i)
    }

    private var afterPerms: (() -> Unit)? = null

    private fun ensurePermissions(then: () -> Unit) {
        val need = mutableListOf<String>()
        // O Nearby exige localização em qualquer versão (erro 8034 se faltar).
        need += Manifest.permission.ACCESS_FINE_LOCATION
        need += Manifest.permission.ACCESS_COARSE_LOCATION
        if (Build.VERSION.SDK_INT >= 31) {
            need += listOf(Manifest.permission.BLUETOOTH_ADVERTISE, Manifest.permission.BLUETOOTH_CONNECT, Manifest.permission.BLUETOOTH_SCAN)
        }
        if (Build.VERSION.SDK_INT >= 33) {
            need += Manifest.permission.NEARBY_WIFI_DEVICES
            need += Manifest.permission.POST_NOTIFICATIONS
        }
        val missing = need.filter { checkSelfPermission(it) != android.content.pm.PackageManager.PERMISSION_GRANTED }
        if (missing.isEmpty()) then() else { afterPerms = then; requestPermissions(missing.toTypedArray(), 1) }
    }

    override fun onRequestPermissionsResult(rc: Int, p: Array<out String>, r: IntArray) {
        if (r.all { it == android.content.pm.PackageManager.PERMISSION_GRANTED }) afterPerms?.invoke()
        else toast("Permissões necessárias para conectar")
        afterPerms = null
    }

    override fun onPause() { Link.uiVisible = false; super.onPause() }

    override fun onResume() { super.onResume(); Link.uiVisible = true; if (prefs.getString("role", null) == "pilot") render() }

    private fun toast(s: String) = Toast.makeText(this, s, Toast.LENGTH_SHORT).show()
}
