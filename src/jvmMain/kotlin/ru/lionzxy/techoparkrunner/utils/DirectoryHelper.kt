package ru.lionzxy.techoparkrunner.utils

import nu.redpois0n.oslib.OperatingSystem
import okio.FileSystem
import okio.Path

object DirectoryHelper {
    fun getDefaultDirectory(): Path {
        val dir = MinecraftDirectoryHelper.getMinecraftDirectory(
            OperatingSystem.getOperatingSystem()
        ).resolve("technomine")
        FileSystem.SYSTEM.createDirectories(dir)
        return dir
    }

    fun getTemporaryDirectory(): Path {
        val dir = getDefaultDirectory().resolve("tmp")
        FileSystem.SYSTEM.createDirectories(dir)
        return dir
    }

    fun getJavaDirectory(): Path {
        val dir = getDefaultDirectory().resolve("jre")
        FileSystem.SYSTEM.createDirectories(dir)
        return dir
    }

    fun getJREPathFile(): Path {
        return getDefaultDirectory().resolve("jrepath.txt")
    }

    fun writeJREPath(jrePath: String) {
        val path = getJREPathFile()
        if (FileSystem.SYSTEM.exists(path)) {
            FileSystem.SYSTEM.delete(path)
        }
        FileSystem.SYSTEM.write(path) {
            writeUtf8(jrePath)
        }
    }

    fun getJREPath(): String {
        val path = getJREPathFile()
        return if (FileSystem.SYSTEM.exists(getJREPathFile())) {
            FileSystem.SYSTEM.read(path) {
                readUtf8()
            }
        } else "java"
    }

    fun getLauncherFile(): Path {
        return getDefaultDirectory().resolve("launcher.jar")
    }

    fun getLauncherOutLogFile(): Path {
        return getDefaultDirectory().resolve("launcherout.log")
    }

    fun getLauncherErrLogFile(): Path {
        return getDefaultDirectory().resolve("launchererr.log")
    }
}