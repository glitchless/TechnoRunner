import sk.tomsik68.mclauncher.impl.common.Platform
import java.io.File

object DirectoryHelper {
    fun getDefaultDirectory(): File {
        val dir = File(Platform.getCurrentPlatform().workingDirectory, "technomine")
        dir.mkdirs()
        return dir
    }

    fun getTemporaryDirectory(): File {
        val dir = File(getDefaultDirectory(), "tmp")
        dir.mkdirs()
        return dir
    }

    fun getJavaDirectory(): File {
        val dir = File(getDefaultDirectory(), "jre")
        dir.mkdirs()
        return dir
    }

    fun getJREPathFile(): File {
        return File(getDefaultDirectory(), "jrepath.txt")
    }

    fun writeJREPath(path: String) {
        getJREPathFile().createWithMkDirs(path)
    }

    fun getLauncherFile(): File {
        return File(getDefaultDirectory(), "launcher.jar")
    }
}

fun File.createWithMkDirs(initialContent: String) {
    parentFile.mkdirs()

    if (exists()) {
        delete()
    }

    if (!createNewFile()) {
        return
    }

    writeText(initialContent)
}