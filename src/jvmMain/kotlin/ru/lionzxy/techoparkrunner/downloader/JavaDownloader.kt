package ru.lionzxy.techoparkrunner.downloader

import io.ktor.client.HttpClient
import io.ktor.client.call.body
import io.ktor.client.request.get
import kotlinx.serialization.SerialName
import kotlinx.serialization.Serializable
import nu.redpois0n.oslib.Arch
import nu.redpois0n.oslib.OperatingSystem
import okhttp3.internal.format
import okio.FileSystem
import okio.Path
import ru.lionzxy.techoparkrunner.model.ProgressState
import ru.lionzxy.techoparkrunner.utils.DirectoryHelper
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import okio.Path.Companion.toPath
import ru.lionzxy.techoparkrunner.utils.download

private const val JRE_JSON_URL = "https://minecraft.glitchless.ru/jres.json"

class JavaDownloader(
    private val client: HttpClient,
    private val onStateUpdate: (ProgressState) -> Unit
) {

    suspend fun exist(): Boolean = withContext(Dispatchers.IO) {
        val jreFile = DirectoryHelper.getJREPathFile()
        if (!FileSystem.SYSTEM.exists(jreFile)) {
            return@withContext false
        }
        val jrePath = FileSystem.SYSTEM.read(jreFile) { this.readUtf8() }
        if (jrePath.isBlank()) {
            return@withContext false
        }

        return@withContext FileSystem.SYSTEM.exists(jrePath.toPath())
    }

    suspend fun download() = withContext(Dispatchers.IO) {
        onStateUpdate(ProgressState("Получаем версии Java..."))
        val jres = client.get(JRE_JSON_URL).body<List<JavaBinaryModel>>()
        println("Receive list of jres: $jres")

        val os = OperatingSystem.getOperatingSystem()
        println("Detected os is $os")
        val jre = jres.find {
            OperatingSystem.getOperatingSystem(it.type) == os.type &&
                Arch.getArch(it.arch) == os.arch
        } ?: error("Can't found jre for ${os.type} and ${os.arch}")
        val jreArchive = downloadJre(jre)
        var unpackedJre = unpackJre(jreArchive, jre)
        if (unpackedJre.isRelative) {
            unpackedJre = unpackedJre.toFile().absolutePath.toPath()
        }
        DirectoryHelper.writeJREPath(unpackedJre.toString())
    }

    private suspend fun downloadJre(
        javaBinaryModel: JavaBinaryModel
    ): Path {
        onStateUpdate(ProgressState("Загрузка Java"))
        val jreFile = DirectoryHelper.getTemporaryDirectory().resolve("jre.${javaBinaryModel.extension}")
        println("Download ${javaBinaryModel.downloadUrl} in $jreFile")
        client.download(javaBinaryModel.downloadUrl, jreFile) { percent ->
            onStateUpdate(ProgressState("Загрузка Java... ${format("%.2f", percent * 100)}%", percent))
        }
        onStateUpdate(ProgressState("Java загружена"))
        return jreFile
    }

    private suspend fun unpackJre(jrePath: Path, jre: JavaBinaryModel): Path {
        onStateUpdate(ProgressState("Разархивирование Java..."))
        val archiver = if (jre.extension.equals("zip", true)) {
            ArchiverFactory.createArchiver(ArchiveFormat.ZIP)
        } else {
            ArchiverFactory.createArchiver(ArchiveFormat.TAR, CompressionType.GZIP)
        }
        archiver.extract(jrePath.toFile(), DirectoryHelper.getJavaDirectory())
        println("Jre directory: ${DirectoryHelper.getJavaDirectory()}")
        onStateUpdate(ProgressState("Java разархивирована"))
        return DirectoryHelper.getJavaDirectory().resolve(jre.javaRelativePath)
    }
}

@Serializable
private data class JavaBinaryModel(
    @SerialName("type")
    val type: String,
    @SerialName("arch")
    val arch: String,
    @SerialName("downloadUrl")
    val downloadUrl: String,
    @SerialName("javaRelativePath")
    var javaRelativePath: String,
    @SerialName("extension")
    var extension: String
)