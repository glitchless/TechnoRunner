package ru.glitchless.games.tprunner.run

import ru.glitchless.games.tprunner.utils.DirectoryHelper

fun runLauncher() {
    val processBuilder =
            ProcessBuilder(DirectoryHelper.getJREPath(), "-jar", DirectoryHelper.getLauncherFile().absolutePath)
    println("Execute command: ${processBuilder.command()}")
    processBuilder.directory(DirectoryHelper.getDefaultDirectory())
    processBuilder.redirectError(DirectoryHelper.getLauncherErrLogFile())
    processBuilder.redirectOutput(DirectoryHelper.getLauncherOutLogFile())

    val proc = processBuilder.start()
    proc.inputStream.bufferedReader().use { br ->
        while (proc.isAlive) {
            br.readLine()?.let { println(it) }
        }
    }
}
