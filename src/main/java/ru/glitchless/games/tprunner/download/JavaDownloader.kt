package ru.glitchless.games.tprunner.download

import com.google.gson.Gson
import com.google.gson.annotations.SerializedName
import com.google.gson.reflect.TypeToken
import nu.redpois0n.oslib.Arch
import nu.redpois0n.oslib.OperatingSystem
import org.rauschig.jarchivelib.ArchiveFormat
import org.rauschig.jarchivelib.ArchiverFactory
import org.rauschig.jarchivelib.CompressionType
import ru.glitchless.games.tprunner.utils.DirectoryHelper
import sk.tomsik68.mclauncher.api.ui.IProgressMonitor
import sk.tomsik68.mclauncher.util.FileUtils
import sk.tomsik68.mclauncher.util.HttpUtils
import java.io.File

private const val JRE_JSON_URL = "https://minecraft.glitchless.ru/jres.json"

class JavaDownloader {
    private val gson = Gson()
    private var javaBinary: JavaBinaryModel? = null

    fun initDownloader() {
        val json = HttpUtils.httpGet(JRE_JSON_URL)
        val os = OperatingSystem.getOperatingSystem()
        javaBinary = gson.fromJson<List<JavaBinaryModel>>(
            json,
            object : TypeToken<List<JavaBinaryModel>>() {}.type
        ).find {
            OperatingSystem.getOperatingSystem(it.type) == os.type &&
                    Arch.getArch(it.arch) == os.arch
        }
    }

    fun downloadJava(monitor: IProgressMonitor): File? {
        if (javaBinary == null) {
            return null
        }

        val jreFile = File(DirectoryHelper.getTemporaryDirectory(), "jre.${javaBinary!!.extension}")
        monitor.setStatus("Загрузка Java...")
        FileUtils.downloadFileWithProgress(javaBinary!!.downloadUrl, jreFile, monitor)
        println("Jre file download: ${jreFile.absoluteFile}")
        monitor.setProgress(100)

        val archiver = if (javaBinary!!.extension.equals("zip", true)) {
            ArchiverFactory.createArchiver(ArchiveFormat.ZIP)
        } else {
            ArchiverFactory.createArchiver(ArchiveFormat.TAR, CompressionType.GZIP)
        }
        archiver.extract(jreFile, DirectoryHelper.getJavaDirectory())
        println("Jre directory: ${DirectoryHelper.getJavaDirectory()}")
        return File(DirectoryHelper.getJavaDirectory(), javaBinary!!.javaRelativePath)
    }
}

data class JavaBinaryModel(
    @SerializedName("type")
    val type: String,
    @SerializedName("arch")
    val arch: String,
    @SerializedName("downloadUrl")
    val downloadUrl: String,
    @SerializedName("javaRelativePath")
    var javaRelativePath: String,
    @SerializedName("extension")
    var extension: String
)
