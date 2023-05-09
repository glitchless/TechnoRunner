package ru.lionzxy.techoparkrunner.utils

import io.ktor.client.HttpClient
import io.ktor.client.plugins.onDownload
import io.ktor.client.request.get
import io.ktor.client.statement.bodyAsChannel
import io.ktor.util.cio.use
import io.ktor.util.cio.writeChannel
import io.ktor.utils.io.copyAndClose
import kotlin.math.max
import kotlin.math.min
import okhttp3.internal.format
import okio.Path

suspend fun HttpClient.download(url: String, to: Path, onPercentUpdate: (Float) -> Unit) {
    val channel = get(url) {
        onDownload { bytesSentTotal, contentLength ->
            val percent = min(bytesSentTotal.toFloat() / max(contentLength, 1), 1.0f)
            println("Download $bytesSentTotal / $contentLength (${format("%.2f", percent * 100)})")
            onPercentUpdate(percent)
        }
    }.bodyAsChannel()
    println("Channel receive")
    to.toFile().writeChannel().use {
        channel.copyAndClose(this)
    }
}