package app.motomsg

import android.Manifest
import android.app.Activity
import android.content.Intent
import android.content.SharedPreferences
import android.graphics.Color
import android.graphics.Typeface
import android.graphics.drawable.GradientDrawable
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.provider.Settings
import android.view.Gravity
import android.view.View
import android.widget.Button
import android.widget.EditText
import android.widget.GridLayout
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.TextView
import android.widget.Toast

/** Paleta do design aprovado: fundo escuro, destaque âmbar. */
object Cores {
    const val FUNDO = 0xFF0F1115.toInt()
    const val CARTAO = 0xFF1B1F27.toInt()
    const val BOTAO = 0xFF2A303B.toInt()
    const val BORDA = 0xFF3D4452.toInt()
    const val AMBAR = 0xFFFFB300.toInt()
    const val TEXTO = 0xFFF4F5F7.toInt()
    const val SUAVE = 0xFFA8B0BD.toInt()
    const val OK = 0xFF3DDC84.toInt()
    const val ESCURO = 0xFF111111.toInt()
}

class MainActivity : Activity() {
    private lateinit var prefs: SharedPreferences
    private lateinit var root: LinearLayout
    private lateinit var statusView: TextView
    private lateinit var statusDot: View
    private val dp get() = resources.displayMetrics.density

    // Mensagens prontas da garupa: (texto, respostas possíveis). Sem respostas = só aviso.
    private val presets = listOf(
        "Pare, por favor" to emptyList<String>(),
        "Mais devagar" to emptyList(),
        "Pode seguir" to emptyList(),
        "Frio / calor" to emptyList(),
        "Podemos parar?" to listOf("Sim", "Não"),
        "Quero ir ao banheiro" to listOf("OK", "Em 10 min"),
        "Está tudo bem?" to listOf("Tudo bem", "Calma!"),
        "Com fome?" to listOf("Sim", "Não"),
    )

    override fun onCreate(b: Bundle?) {
        super.onCreate(b)
        prefs = getSharedPreferences("p", MODE_PRIVATE)
        window.statusBarColor = Cores.FUNDO
        window.navigationBarColor = Cores.FUNDO
        root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(pad(20), pad(48), pad(20), pad(20))
        }
        setContentView(ScrollView(this).apply {
            setBackgroundColor(Cores.FUNDO); isFillViewport = true; addView(root)
        })
        statusView = TextView(this).apply { textSize = 20f; setTextColor(Cores.TEXTO); typeface = Typeface.DEFAULT_BOLD }
        statusDot = View(this)
        Link.onStatus = { runOnUiThread { refreshStatus() } }
        Link.onMessage = { m -> runOnUiThread { showAnswer(m) } }
        render()
    }

    override fun onDestroy() { Link.onStatus = null; Link.onMessage = null; super.onDestroy() }

    private fun pad(v: Int) = (v * dp).toInt()

    private fun round(color: Int, radius: Int, stroke: Int = 0, strokeColor: Int = 0) = GradientDrawable().apply {
        setColor(color); cornerRadius = radius * dp
        if (stroke > 0) setStroke(pad(stroke), strokeColor)
    }

    private fun text(s: String, size: Float, color: Int = Cores.TEXTO, bold: Boolean = false) = TextView(this).apply {
        text = s; textSize = size; setTextColor(color); setPadding(0, pad(6), 0, pad(6))
        if (bold) typeface = Typeface.DEFAULT_BOLD
        root.addView(this)
    }

    private fun spacer() = root.addView(View(this), LinearLayout.LayoutParams(1, 0, 1f))

    private fun styled(label: String, bg: Int, fg: Int, size: Float, h: Int, stroke: Int = 0, onClick: () -> Unit) =
        Button(this).apply {
            text = label; textSize = size; setTextColor(fg); isAllCaps = false; typeface = Typeface.DEFAULT_BOLD
            background = round(bg, 16, stroke, fg); stateListAnimator = null
            setOnClickListener { onClick() }
        }.also { it.tag = h }

    private fun big(label: String, bg: Int, fg: Int, size: Float, h: Int, stroke: Int = 0, onClick: () -> Unit): Button {
        val b = styled(label, bg, fg, size, h, stroke, onClick)
        root.addView(b, LinearLayout.LayoutParams(-1, pad(h)).apply { topMargin = pad(10) })
        return b
    }

    private fun refreshStatus() {
        statusView.text = Link.status
        val on = Link.status.startsWith("Conectado")
        statusDot.background = round(if (on) Cores.OK else Cores.AMBAR, 8)
    }

    private fun statusCard() {
        val card = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL; gravity = Gravity.CENTER_VERTICAL
            background = round(Cores.CARTAO, 14); setPadding(pad(16), pad(16), pad(16), pad(16))
        }
        (statusDot.parent as? android.view.ViewGroup)?.removeView(statusDot)
        (statusView.parent as? android.view.ViewGroup)?.removeView(statusView)
        card.addView(statusDot, LinearLayout.LayoutParams(pad(16), pad(16)).apply { rightMargin = pad(12) })
        card.addView(statusView)
        root.addView(card, LinearLayout.LayoutParams(-1, -2).apply { topMargin = pad(8) })
        refreshStatus()
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
        text("MotoMsg", 40f, bold = true)
        text("Mensagens grandes no GPS do piloto, sem internet.", 16f, Cores.SUAVE)
        text("Código do par (igual nos dois celulares)", 14f, Cores.SUAVE)
        val code = EditText(this).apply {
            setText(prefs.getString("code", "1234")); textSize = 36f; typeface = Typeface.DEFAULT_BOLD
            setTextColor(Cores.TEXTO); gravity = Gravity.CENTER; letterSpacing = 0.3f
            background = round(Cores.CARTAO, 14, 2, Cores.BORDA)
            inputType = android.text.InputType.TYPE_CLASS_NUMBER
            root.addView(this, LinearLayout.LayoutParams(-1, pad(72)))
        }
        fun pick(role: String) {
            prefs.edit().putString("role", role).putString("code", code.text.toString().ifBlank { "1234" }).apply()
            render()
        }
        spacer()
        big("Sou o PILOTO\ncelular com o GPS", Cores.AMBAR, Cores.ESCURO, 24f, 104) { pick("pilot") }
        big("Sou a GARUPA\nenvia mensagens", Cores.BOTAO, Cores.TEXTO, 24f, 104) { pick("pillion") }
    }

    // ---------- Piloto ----------
    private fun pilotScreen() {
        text("Piloto · código ${prefs.getString("code", "")}", 28f, bold = true)
        statusCard()
        if (!Settings.canDrawOverlays(this)) {
            text("Falta permitir \"Sobrepor a outros apps\" para a mensagem aparecer por cima do GPS.", 15f, 0xFFFFD580.toInt())
            big("Permitir sobreposição", Color.TRANSPARENT, Cores.AMBAR, 20f, 64, 2) {
                startActivity(Intent(Settings.ACTION_MANAGE_OVERLAY_PERMISSION, Uri.parse("package:$packageName")))
            }
        }
        spacer()
        big("Ligar", Cores.AMBAR, Cores.ESCURO, 26f, 88) { ensurePermissions { startLink(true) } }
        big("Testar tela grande", Cores.BOTAO, Cores.TEXTO, 20f, 64) {
            if (!Settings.canDrawOverlays(this)) toast("Permita a sobreposição primeiro")
            else Overlay(this) { _, a -> toast("Resposta: $a") }
                .show(Msg("msg", "t", "Podemos parar?", listOf("Sim", "Não")))
        }
        val row = LinearLayout(this).apply { orientation = LinearLayout.HORIZONTAL }
        row.addView(styled("Desligar", Cores.CARTAO, Cores.TEXTO, 17f, 56) { stopService(Intent(this, LinkService::class.java)) },
            LinearLayout.LayoutParams(0, pad(56), 1f).apply { rightMargin = pad(6) })
        row.addView(styled("Trocar papel", Cores.CARTAO, Cores.TEXTO, 17f, 56) { prefs.edit().remove("role").apply(); render() },
            LinearLayout.LayoutParams(0, pad(56), 1f).apply { leftMargin = pad(6) })
        root.addView(row, LinearLayout.LayoutParams(-1, -2).apply { topMargin = pad(10) })
    }

    // ---------- Garupa ----------
    private fun pillionScreen() {
        text("Garupa · código ${prefs.getString("code", "")}", 28f, bold = true)
        statusCard()
        big("Conectar", Cores.AMBAR, Cores.ESCURO, 22f, 64) { ensurePermissions { startLink(false) } }
        grid(presets.filter { it.second.isEmpty() })
        text("Perguntas — o piloto responde tocando", 14f, Cores.SUAVE)
        grid(presets.filter { it.second.isNotEmpty() })
        spacer()
        val row = LinearLayout(this).apply { orientation = LinearLayout.HORIZONTAL }
        val free = EditText(this).apply {
            hint = "Escrever mensagem…"; setHintTextColor(Cores.SUAVE); setTextColor(Cores.TEXTO); textSize = 18f
            setPadding(pad(14), 0, pad(14), 0); background = round(Cores.CARTAO, 14, 2, Cores.BORDA)
        }
        fun sendFree(opts: List<String>) {
            free.text.toString().trim().takeIf { it.isNotEmpty() }?.let { send(it, opts); free.setText("") }
        }
        row.addView(free, LinearLayout.LayoutParams(0, pad(60), 1f))
        row.addView(styled("Enviar", Cores.AMBAR, Cores.ESCURO, 18f, 60) { sendFree(emptyList()) },
            LinearLayout.LayoutParams(pad(96), pad(60)).apply { leftMargin = pad(10) })
        root.addView(row, LinearLayout.LayoutParams(-1, -2).apply { topMargin = pad(10) })
        big("Enviar e pedir Sim/Não", Cores.CARTAO, Cores.TEXTO, 15f, 48) { sendFree(listOf("Sim", "Não")) }
        big("Trocar papel", Color.TRANSPARENT, Cores.SUAVE, 14f, 44) {
            stopService(Intent(this, LinkService::class.java)); prefs.edit().remove("role").apply(); render()
        }
    }

    private fun grid(items: List<Pair<String, List<String>>>) {
        val g = GridLayout(this).apply { columnCount = 2 }
        for ((t, opts) in items) {
            val ask = opts.isNotEmpty()
            val label = if (ask) t + "\n" + opts.joinToString(" · ") else t
            val b = styled(label, if (ask) Color.TRANSPARENT else Cores.BOTAO, if (ask) Cores.AMBAR else Cores.TEXTO, 18f, 92, if (ask) 2 else 0) { send(t, opts) }
            g.addView(b, GridLayout.LayoutParams(
                GridLayout.spec(GridLayout.UNDEFINED, 1f), GridLayout.spec(GridLayout.UNDEFINED, 1f),
            ).apply { width = 0; height = pad(92); setMargins(pad(5), pad(5), pad(5), pad(5)) })
        }
        root.addView(g, LinearLayout.LayoutParams(-1, -2))
    }

    private fun send(t: String, opts: List<String>) {
        val ok = Link.send?.invoke(Msg("msg", System.currentTimeMillis().toString(), t, opts)) ?: false
        toast(if (ok) "Enviado" else "Não conectado")
    }

    private fun showAnswer(m: Msg) {
        val box = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL; setPadding(pad(24), pad(28), pad(24), pad(24))
            background = round(Cores.CARTAO, 28)
        }
        box.addView(TextView(this).apply { text = "Resposta do piloto"; textSize = 15f; setTextColor(Cores.SUAVE) })
        box.addView(TextView(this).apply {
            text = m.text; textSize = 72f; typeface = Typeface.DEFAULT_BOLD; gravity = Gravity.CENTER
            setTextColor(Cores.AMBAR); setPadding(0, pad(24), 0, pad(24))
        })
        val d = android.app.AlertDialog.Builder(this).setView(box).create()
        box.addView(styled("OK", Cores.AMBAR, Cores.ESCURO, 24f, 72) { d.dismiss() },
            LinearLayout.LayoutParams(-1, pad(72)))
        d.window?.setBackgroundDrawable(android.graphics.drawable.ColorDrawable(Color.TRANSPARENT))
        d.show()
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
