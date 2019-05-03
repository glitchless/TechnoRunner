package downloader

import ru.lionzxy.tplauncher.downloader.javas.JavaDownloader
import sk.tomsik68.mclauncher.api.ui.IProgressMonitor
import java.io.File

object CheckAndDownload {
    fun checkAndDownloadAll(monitor: IProgressMonitor) {
        val jre = DirectoryHelper.getJREPathFile()
        if (!jre.exists() || jre.readText().isEmpty() || !File(jre.readText()).exists()) {
            val javaDownloader = JavaDownloader()
            javaDownloader.initDownloader()
            javaDownloader.downloadJava(monitor)?.let {
                DirectoryHelper.writeJREPath(it.absolutePath)
            }
        }

        val launcherDownloader = LauncherDownloader()
        launcherDownloader.initDownloader()
        if (!launcherDownloader.checkFile()) {
            launcherDownloader.updateLauncher(monitor)
        }
    }
}