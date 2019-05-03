import downloader.CheckAndDownload
import java.awt.Color
import java.io.File
import javax.swing.UIManager

fun main() {
    UIManager.put("ProgressBar.background", Color.WHITE)
    UIManager.put("ProgressBar.foreground", Color.GREEN)
    UIManager.put("ProgressBar.selectionBackground", Color.WHITE)
    UIManager.put("ProgressBar.selectionForeground", Color.GREEN)
    val splash = utils.SplashScreen()
    splash.display()
    CheckAndDownload.checkAndDownloadAll(splash)
    splash.stop()

    var javaPath: String? = if (DirectoryHelper.getJREPathFile().exists()) {
        DirectoryHelper.getJREPathFile().readText()
    } else null

    if (javaPath.isNullOrEmpty() || !File(javaPath).exists()) {
        javaPath = "java"
    }

    val processBuilder =
            ProcessBuilder(javaPath, "-jar", DirectoryHelper.getLauncherFile().absolutePath)
    processBuilder.directory(DirectoryHelper.getDefaultDirectory())
    processBuilder.redirectError(File("launchererr.log"))
    processBuilder.redirectOutput(File("launcherout.log"))
    val proc = processBuilder.start()
    proc.inputStream.bufferedReader().use { br ->
        while (proc.isAlive) {
            br.readLine()?.let { println(it) }
        }
    }
    System.exit(0)
}