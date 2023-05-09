package ru.lionzxy.techoparkrunner.utils

import nu.redpois0n.oslib.AbstractOperatingSystem
import nu.redpois0n.oslib.OperatingSystem
import okio.Path
import okio.Path.Companion.toPath
import java.io.File


private const val MINECRAFT_FOLDER_NAME = ".minecraft"

object MinecraftDirectoryHelper {
    fun getMinecraftDirectory(operatingSystem: AbstractOperatingSystem): Path {
        return when (operatingSystem.type) {
            OperatingSystem.WINDOWS -> getWindowsDirectory()
            OperatingSystem.MACOS -> getMacOSDirectory()
            OperatingSystem.LINUX,
            OperatingSystem.SOLARIS,
            OperatingSystem.BSD,
            OperatingSystem.UNKNOWN -> getLinuxDirectory()

            else -> getLinuxDirectory()
        } ?: System.getProperty("user.home").toPath().resolve(MINECRAFT_FOLDER_NAME)
    }

    private fun getWindowsDirectory(): Path? {
        val homeDir = System.getenv("APPDATA") ?: System.getProperty("user.home")
        return homeDir?.toPath()?.resolve(MINECRAFT_FOLDER_NAME)
    }

    private fun getLinuxDirectory(): Path? {
        return System.getProperty("user.home")?.toPath()?.resolve(MINECRAFT_FOLDER_NAME)
    }

    private fun getMacOSDirectory(): Path? {
        return System.getProperty("user.home")?.toPath()?.resolve("Library/Application Support/minecraft")
    }
}