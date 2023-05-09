package ru.lionzxy.techoparkrunner.downloader

import io.ktor.client.HttpClient
import io.ktor.client.call.body
import io.ktor.client.plugins.onDownload
import io.ktor.client.request.get
import io.ktor.client.statement.bodyAsChannel
import io.ktor.util.cio.use
import io.ktor.util.cio.writeChannel
import io.ktor.utils.io.copyAndClose
import kotlinx.serialization.SerialName
import kotlinx.serialization.Serializable
import java.io.File
import kotlin.math.max
import kotlin.math.min
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import okhttp3.internal.format
import okio.FileSystem
import ru.lionzxy.techoparkrunner.model.ProgressState
import ru.lionzxy.techoparkrunner.utils.DirectoryHelper
import ru.lionzxy.techoparkrunner.utils.HashUtils
import ru.lionzxy.techoparkrunner.utils.download

private const val LAUNCHER_JSON_URL = "https://minecraft.glitchless.ru/launcher.json"

class LauncherDownloader(
    private val client: HttpClient,
    private val onStateUpdate: (ProgressState) -> Unit
) {
    suspend fun shouldDownload(): Boolean = withContext(Dispatchers.IO) {
        if (!FileSystem.SYSTEM.exists(DirectoryHelper.getLauncherFile())) {
            return@withContext true
        }

        onStateUpdate(ProgressState("Проверка обновлений лаунчера..."))

        val launcher = try {
            client.get(LAUNCHER_JSON_URL).body<LauncherModel>()
        } catch (e: Exception) {
            e.printStackTrace()
            return@withContext false
        }

        val sha256 = HashUtils.generateSHA256(DirectoryHelper.getLauncherFile())
        println("Calculated sha256 is $sha256, received is ${launcher.sha256}")
        return@withContext sha256 != launcher.sha256
    }

    suspend fun download() = withContext(Dispatchers.IO) {
        onStateUpdate(ProgressState("Загрузка обновления лаунчера..."))
        val launcher = client.get(LAUNCHER_JSON_URL).body<LauncherModel>()
        val launcherJar = DirectoryHelper.getTemporaryDirectory().resolve("update_launcher.jar")
        client.download(launcher.downloadUrl, launcherJar) { percent ->
            onStateUpdate(ProgressState("Загрузка обновления лаунчера... ${format("%.2f", percent * 100)}%", percent))
        }
        if (HashUtils.generateSHA256(launcherJar) != launcher.sha256) {
            error("Incorrect hashes")
        }
        onStateUpdate(ProgressState("Загрузка завершена, установка..."))

        val targetJar = DirectoryHelper.getLauncherFile()
        if (FileSystem.SYSTEM.exists(targetJar)) {
            FileSystem.SYSTEM.delete(targetJar)
        }
        FileSystem.SYSTEM.atomicMove(launcherJar, targetJar)
        onStateUpdate(ProgressState("Лаунчер установлен"))
    }
}

@Serializable
private data class LauncherModel(
    @SerialName("version")
    val version: String,
    @SerialName("downloadFullPath")
    val downloadUrl: String,
    @SerialName("SHA-256")
    val sha256: String
)