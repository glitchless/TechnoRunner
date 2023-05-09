package ru.lionzxy.techoparkrunner.utils.unzip

import kotlinx.coroutines.delay
import net.lingala.zip4j.ZipFile
import net.lingala.zip4j.progress.ProgressMonitor
import okio.Path


private const val UNZIP_REQUEST_TIMEOUT_MS = 100L

object UnzipWithProgress {
    suspend fun unzipWithProgress(path: Path, to: Path, onPercentUpdate: (Float) -> Unit) {
        println("Unzip $path to $to")

        val zipFile = ZipFile(path.toFile())
        zipFile.isRunInThread = true
        zipFile.extractAll(to.toFile().absolutePath)
        while (zipFile.progressMonitor.state == ProgressMonitor.State.BUSY) {
            var percent = zipFile.progressMonitor.percentDone.toFloat() / 100
            if (percent < 0.0f) {
                percent = 0.0f
            } else if (percent > 1.0f) {
                percent = 1.0f
            }
            onPercentUpdate(percent)
            delay(UNZIP_REQUEST_TIMEOUT_MS)
        }
    }
}