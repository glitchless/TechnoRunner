package ru.lionzxy.techoparkrunner.utils

import io.ktor.client.*
import io.ktor.client.plugins.*
import io.ktor.client.request.*
import io.ktor.client.statement.*
import io.ktor.util.cio.*
import io.ktor.utils.io.*
import okio.Path
import kotlin.math.max
import kotlin.math.min

suspend fun HttpClient.download(url: String, to: Path, onPercentUpdate: (Float) -> Unit) {
    val channel = get(url) {
        onDownload { bytesSentTotal, contentLength ->
            val percent = min(bytesSentTotal.toFloat() / max(contentLength, 1), 1.0f)
            onPercentUpdate(percent)
        }
    }.bodyAsChannel()
    println("Channel receive")
    to.toFile().writeChannel().use {
        channel.copyAndClose(this)
    }
}