package downloader

import DirectoryHelper
import com.google.gson.Gson
import sk.tomsik68.mclauncher.api.ui.IProgressMonitor
import sk.tomsik68.mclauncher.util.FileUtils
import sk.tomsik68.mclauncher.util.HttpUtils
import utils.Utils
import java.io.File

private const val LAUNCHER_JSON_URL = "https://minecraft.glitchless.ru/launcher.json"

class LauncherDownloader {
    val gson = Gson()
    var launcherModel: LauncherModel? = null

    fun initDownloader() {
        try {
            val json = HttpUtils.httpGet(LAUNCHER_JSON_URL)
            launcherModel = gson.fromJson(json, LauncherModel::class.java)
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    fun checkFile(): Boolean {
        if(!DirectoryHelper.getLauncherFile().exists()) {
            return false
        }

        if (launcherModel == null) {
            return true
        }

        val sha256 = Utils.generateSHA256(DirectoryHelper.getLauncherFile())
        return sha256 == launcherModel!!.sha256
    }

    fun updateLauncher(monitor: IProgressMonitor)  {
        if (launcherModel == null) {
            return
        }

        monitor.setStatus("Downloading launcher...")
        val updateFile = File(DirectoryHelper.getTemporaryDirectory(), "update_launcher.jar");
        FileUtils.downloadFileWithProgress(launcherModel!!.downloadUrl, updateFile, monitor)

        if (Utils.generateSHA256(updateFile) != launcherModel!!.sha256) {
            return
        }

        if (!DirectoryHelper.getLauncherFile().exists() || DirectoryHelper.getLauncherFile().delete()) {
            updateFile.renameTo(DirectoryHelper.getLauncherFile())
        }
    }
}