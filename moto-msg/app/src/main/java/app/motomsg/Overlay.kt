package app.motomsg

import android.content.Context
import android.graphics.Color
import android.graphics.PixelFormat
import android.graphics.Typeface
import android.view.Gravity
import android.view.View
import android.view.WindowManager
import android.widget.Button
import android.widget.LinearLayout
import android.widget.TextView
import android.util.TypedValue

/** Texto gigante por cima de qualquer app (GPS). Tocar no texto fecha; tocar numa opção responde. */
class Overlay(private val ctx: Context, private val onReply: (Msg, String) -> Unit) {
    private val wm = ctx.getSystemService(Context.WINDOW_SERVICE) as WindowManager
    private var view: View? = null

    fun show(m: Msg) {
        dismiss()
        val dp = ctx.resources.displayMetrics.density

        val text = TextView(ctx).apply {
            this.text = m.text
            setTextColor(Color.WHITE)
            typeface = Typeface.DEFAULT_BOLD
            gravity = Gravity.CENTER
            setAutoSizeTextTypeUniformWithConfiguration(28, 200, 2, TypedValue.COMPLEX_UNIT_SP)
        }
        val hint = TextView(ctx).apply {
            this.text = if (m.options.isEmpty()) "toque para fechar" else "toque numa resposta"
            setTextColor(Color.LTGRAY)
            textSize = 16f
            gravity = Gravity.CENTER
        }
        val root = LinearLayout(ctx).apply {
            orientation = LinearLayout.VERTICAL
            setBackgroundColor(0xF5000000.toInt())
            setPadding((16 * dp).toInt(), (48 * dp).toInt(), (16 * dp).toInt(), (32 * dp).toInt())
            addView(text, LinearLayout.LayoutParams(-1, 0, 1f))
            addView(hint, LinearLayout.LayoutParams(-1, -2))
            // Tocar em qualquer lugar fora dos botões fecha a mensagem.
            setOnClickListener { dismiss() }
        }
        if (m.options.isNotEmpty()) {
            val row = LinearLayout(ctx).apply { orientation = LinearLayout.HORIZONTAL }
            for (opt in m.options) {
                val first = opt == m.options.first()
                val b = Button(ctx).apply {
                    this.text = opt
                    textSize = 40f
                    typeface = Typeface.DEFAULT_BOLD
                    isAllCaps = false
                    stateListAnimator = null
                    setTextColor(if (first) 0xFF111111.toInt() else Color.WHITE)
                    background = android.graphics.drawable.GradientDrawable().apply {
                        setColor(if (first) 0xFFFFB300.toInt() else 0xFF3D4452.toInt()); cornerRadius = 24 * dp
                    }
                    setOnClickListener { onReply(m, opt); dismiss() }
                }
                row.addView(b, LinearLayout.LayoutParams(0, (160 * dp).toInt(), 1f).apply {
                    setMargins((6 * dp).toInt(), (12 * dp).toInt(), (6 * dp).toInt(), 0)
                })
            }
            root.addView(row, LinearLayout.LayoutParams(-1, -2))
        }

        val lp = WindowManager.LayoutParams(
            -1, -1,
            WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY,
            WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE or
                WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON or
                WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN,
            PixelFormat.TRANSLUCENT,
        )
        wm.addView(root, lp)
        view = root
    }

    fun dismiss() {
        view?.let { runCatching { wm.removeView(it) } }
        view = null
    }
}
