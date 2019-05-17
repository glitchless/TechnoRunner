package ru.glitchless.games.tprunner

import ru.glitchless.games.tprunner.download.checkAndDownloadAll
import ru.glitchless.games.tprunner.run.runLauncher
import ru.glitchless.games.tprunner.ui.SplashScreen
import ru.glitchless.games.tprunner.ui.prepareUI

fun main() {
    prepareUI()

    val splash = SplashScreen()
    splash.display()
    checkAndDownloadAll(splash)
    splash.stop()

    runLauncher()
    System.exit(0)
}
