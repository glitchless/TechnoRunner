package ru.lionzxy.techoparkrunner.utils.unzip

import okio.FileSystem
import okio.Path
import okio.Path.Companion.toOkioPath
import org.apache.commons.compress.archivers.ArchiveEntry
import org.apache.commons.compress.archivers.ArchiveInputStream
import org.apache.commons.compress.archivers.tar.TarArchiveInputStream
import org.apache.commons.compress.archivers.zip.ZipArchiveInputStream
import org.apache.commons.compress.compressors.gzip.GzipCompressorInputStream

class DecompressWithProgress(
    private val fs: FileSystem = FileSystem.SYSTEM
) {
    suspend fun decompressWithProgress(
        path: Path,
        to: Path,
        onPercentUpdate: (Float) -> Unit
    ) {
        println("Untar $path to $to")
        val totalSize = fs.metadataOrNull(path)?.size
        fs.read(path) {
            val stream = if (path.toFile().extension == "zip") {
                ZipArchiveInputStream(inputStream())
            } else if (path.toString().endsWith("tar.gz")) {
                TarArchiveInputStream(GzipCompressorInputStream(inputStream()))
            } else {
                error("Unsupported format for $path")
            }

            stream.use { archiveStream ->
                unsafeUnarchiveStream(archiveStream, to) { bytesRead ->
                    if (totalSize == null) {
                        return@unsafeUnarchiveStream
                    }
                    var percent = (bytesRead.toDouble() / totalSize.toDouble()).toFloat()
                    if (percent < 0.0f) {
                        percent = 0.0f
                    } else if (percent > 1.0f) {
                        percent = 1.0f
                    }
                    onPercentUpdate(percent)
                }
            }
        }
    }

    private suspend fun unsafeUnarchiveStream(
        archiveStream: ArchiveInputStream,
        to: Path,
        onBytesRead: (Long) -> Unit
    ) {
        var entry: ArchiveEntry?
        while (archiveStream.nextEntry.also { entry = it } != null) {
            val currentEntry = entry!!
            if (!archiveStream.canReadEntryData(entry)) {
                println("Can't read entry $entry")
                continue
            }
            val output: Path = to.resolve(currentEntry.name)
            if (currentEntry.isDirectory) {
                if (fs.exists(output) && !fs.isDirectory(output)) {
                    error("failed to create directory $output")
                } else {
                    fs.createDirectories(output)
                }
            } else {
                val parent = output.parent ?: output.toFile().absoluteFile.parentFile.toOkioPath()
                if (fs.exists(parent) && !fs.isDirectory(parent)) {
                    error("failed to create directory $parent")
                } else {
                    fs.createDirectories(parent)
                }

                fs.write(output) {
                    archiveStream.copyTo(outputStream())
                }
            }
            onBytesRead(archiveStream.bytesRead)
        }
    }
}

private fun FileSystem.isDirectory(path: Path) = metadataOrNull(path)?.isDirectory == true
