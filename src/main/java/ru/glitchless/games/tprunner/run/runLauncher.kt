package ru.glitchless.games.tprunner.run

import nu.redpois0n.oslib.OperatingSystem
import ru.glitchless.games.tprunner.utils.DirectoryHelper

fun runLauncher() {
    val processBuilder = ProcessBuilder(getDetachCommand() + getRunCommand())
    println("Execute command: ${processBuilder.command()}")
    processBuilder.directory(DirectoryHelper.getDefaultDirectory())
    processBuilder.redirectError(DirectoryHelper.getLauncherErrLogFile())
    processBuilder.redirectOutput(DirectoryHelper.getLauncherOutLogFile())

    processBuilder.start()
}

fun detachLauncherAndDie() {
    System.exit(0)
}

private fun getDetachCommand(): List<String> {
    return if (OperatingSystem.getOperatingSystem().isUnix) listOf("nohup") else listOf("cmd.exe", "/C", "start")
}

private fun getRunCommand(): List<String> {
    return listOf(DirectoryHelper.getJREPath(), "-jar", DirectoryHelper.getLauncherFile().absolutePath)
}
