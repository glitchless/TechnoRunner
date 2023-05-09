package ru.lionzxy.techoparkrunner.utils

import java.security.MessageDigest
import kotlin.io.encoding.Base64
import kotlin.io.encoding.ExperimentalEncodingApi
import kotlin.math.sin
import okio.Buffer
import okio.FileSystem
import okio.HashingSink
import okio.Path
import okio.Sink
import okio.blackholeSink


@OptIn(ExperimentalEncodingApi::class)
object HashUtils {
    @Throws(Exception::class)
    fun generateSHA256(path: Path): String {
        val hash = HashingSink.sha256(blackholeSink()).use { sink ->
            FileSystem.SYSTEM.read(path) {
                this.readAll(sink)
            }
            sink.hash
        }
        return Base64.encode(hash.toByteArray())
    }
}

