package ru.glitchless.games.tprunner

import ru.glitchless.games.tprunner.download.checkAndDownloadAll
import ru.glitchless.games.tprunner.run.detachLauncherAndDie
import ru.glitchless.games.tprunner.run.runLauncher
import ru.glitchless.games.tprunner.ui.SplashScreen
import sk.tomsik68.mclauncher.api.ui.IProgressMonitor
import java.lang.Thread.sleep
import kotlin.math.pow

fun main() {
    val splash = SplashScreen()
    splash.display()
    val result = tryExponential(10, splash) {
        checkAndDownloadAll(splash)
    }
    if (!result) {
        splash.setStatus("Ошибка при загрузке. Проверьте подключение интернета")
    }
    splash.stop()

    runLauncher()
    detachLauncherAndDie()
}

fun tryExponential(
    attemptsNumber: Int,
    progressMonitor: IProgressMonitor,
    block: () -> Unit
): Boolean {
    var currentAttempt = 0
    while (currentAttempt < attemptsNumber) {
        try {
            block()
            return true
        } catch (ex: Exception) {
            println("Error while try execute task")
            ex.printStackTrace()
        }
        val nextTryMs = 1000L * 2.toDouble().pow(currentAttempt.toDouble()).toLong()
        val startWaitingTime = System.currentTimeMillis()
        do {
            val waitTimeMs = (startWaitingTime + nextTryMs) - System.currentTimeMillis()
            val waitTimeSec = (waitTimeMs / 1000L).toInt()
            progressMonitor.setStatus("Ошибка при загрузке. Попытка $currentAttempt/$attemptsNumber (${waitTimeSec}с)")
            sleep(1000L)
        } while (waitTimeSec > 0)
        currentAttempt++
    }
    return false
}
