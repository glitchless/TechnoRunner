package ru.lionzxy.techoparkrunner.downloader

import io.ktor.client.*
import io.ktor.client.engine.okhttp.*
import io.ktor.client.plugins.contentnegotiation.*
import io.ktor.client.request.*
import io.ktor.serialization.kotlinx.json.*
import kotlin.math.pow
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.collect
import kotlinx.coroutines.withContext
import nu.redpois0n.oslib.OperatingSystem
import ru.lionzxy.techoparkrunner.model.ProgressState

private const val ATTEMPTS_COUNT = 10

class Downloader(
    private val onChangeState: (ProgressState) -> Unit
) {
    private val client = HttpClient(OkHttp) {
        install(ContentNegotiation) {
            json()
        }
    }
    private val javaDownloader = JavaDownloader(client, onChangeState)
    private val launcherDownloader = LauncherDownloader(client, onChangeState)

    suspend fun download(): Boolean {
        val result = tryExponential {
            downloadInternal()
        }
        if (!result) {
            onChangeState(ProgressState("Ошибка при загрузке. Проверьте подключение интернета"))
        }
        return result
    }

    private suspend fun downloadInternal() = withContext(Dispatchers.IO) {
        val os = OperatingSystem.getOperatingSystem()
        println("Init download for ${os.type} ${os.arch}")

        if (!javaDownloader.exist()) {
            javaDownloader.download()
        }
        if (launcherDownloader.shouldDownload()) {
            launcherDownloader.download()
        }
    }

    private suspend fun tryExponential(
        block: suspend () -> Unit
    ): Boolean {
        var currentAttempt = 0
        while (currentAttempt < ATTEMPTS_COUNT) {
            try {
                block()
                return true
            } catch (ex: Exception) {
                println("Error while try execute task")
                ex.printStackTrace()
            }
            val nextTryMs = 1000L * 2.toDouble().pow(currentAttempt.toDouble()).toLong()
            val startWaitingTime = System.currentTimeMillis()
            do {
                val waitTimeMs = (startWaitingTime + nextTryMs) - System.currentTimeMillis()
                val waitTimeSec = (waitTimeMs / 1000L).toInt()
                onChangeState(ProgressState("Ошибка при загрузке. Попытка $currentAttempt/$ATTEMPTS_COUNT через ${waitTimeSec}с"))
                delay(1000L)
            } while (waitTimeSec > 0)
            currentAttempt++
        }
        return false
    }
}