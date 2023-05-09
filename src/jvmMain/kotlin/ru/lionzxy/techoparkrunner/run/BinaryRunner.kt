package ru.lionzxy.techoparkrunner.run

import nu.redpois0n.oslib.OperatingSystem
import ru.lionzxy.techoparkrunner.utils.DirectoryHelper


object BinaryRunner {
    fun runLauncherAndDie() {
        val processBuilder = ProcessBuilder(getCommand())
        println("Execute command: ${processBuilder.command()}")
        processBuilder.directory(DirectoryHelper.getDefaultDirectory().toFile())
        processBuilder.redirectError(DirectoryHelper.getLauncherErrLogFile().toFile())
        processBuilder.redirectOutput(DirectoryHelper.getLauncherOutLogFile().toFile())

        processBuilder.start()
        detachLauncherAndDie()
    }

    private fun detachLauncherAndDie() {
        System.exit(0)
    }
}

private fun getCommand(): List<String> {
    if (OperatingSystem.getOperatingSystem().isUnix) {
        return listOf("nohup") + getRunCommand()
    } else {
        return listOf("cmd.exe", "/C") + getRunCommand()
    }
}

private fun getRunCommand(): List<String> {
    return listOf(DirectoryHelper.getJREPath(), "-jar", DirectoryHelper.getLauncherFile().toFile().absolutePath)
}
