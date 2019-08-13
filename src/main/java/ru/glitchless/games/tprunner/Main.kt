package ru.glitchless.games.tprunner

import ru.glitchless.games.tprunner.download.checkAndDownloadAll
import ru.glitchless.games.tprunner.run.detachLauncherAndDie
import ru.glitchless.games.tprunner.run.runLauncher
import ru.glitchless.games.tprunner.ui.SplashScreen

fun main() {
    val splash = SplashScreen()
    splash.display()
    //checkAndDownloadAll(splash)
    //splash.stop()

    //runLauncher()
    //detachLauncherAndDie()
}
