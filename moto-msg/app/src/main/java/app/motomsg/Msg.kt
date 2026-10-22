package app.motomsg

import org.json.JSONArray
import org.json.JSONObject

/** Mensagem trocada entre os dois celulares. kind = "msg" (garupa -> piloto) ou "reply" (resposta). */
data class Msg(
    val kind: String,
    val id: String,
    val text: String,
    val options: List<String> = emptyList(),
) {
    fun toBytes(): ByteArray = JSONObject()
        .put("k", kind).put("id", id).put("t", text)
        .put("o", JSONArray(options))
        .toString().toByteArray()

    companion object {
        fun fromBytes(b: ByteArray): Msg {
            val j = JSONObject(String(b))
            val o = j.optJSONArray("o")
            return Msg(
                j.getString("k"), j.getString("id"), j.getString("t"),
                List(o?.length() ?: 0) { o!!.getString(it) },
            )
        }
    }
}
