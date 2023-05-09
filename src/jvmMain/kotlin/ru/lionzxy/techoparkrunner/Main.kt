package ru.lionzxy.techoparkrunner

import androidx.compose.foundation.layout.width
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.DpSize
import androidx.compose.ui.unit.dp
import androidx.compose.ui.window.Window
import androidx.compose.ui.window.WindowPosition
import androidx.compose.ui.window.application
import androidx.compose.ui.window.rememberWindowState
import ru.lionzxy.techoparkrunner.downloader.Downloader
import ru.lionzxy.techoparkrunner.model.ProgressState
import ru.lionzxy.techoparkrunner.run.BinaryRunner
import ru.lionzxy.techoparkrunner.ui.ComposableLoadingScreen

fun main() {
    application {
        val windowState = rememberWindowState(
            size = DpSize.Unspecified,
            position = WindowPosition.Aligned(Alignment.Center)
        )
        var progressState by remember { mutableStateOf(ProgressState(text = "Загрузка...", progress = null)) }
        val downloader = remember {
            Downloader {
                progressState = it
            }
        }
        Window(
            state = windowState,
            onCloseRequest = ::exitApplication,
            resizable = false,
            undecorated = true
        ) {
            ComposableLoadingScreen(
                modifier = Modifier.width(480.dp),
                progressState = progressState,
                onClose = ::exitApplication
            )
        }

        LaunchedEffect(downloader) {
            downloader.download()
            BinaryRunner.runLauncherAndDie()
        }
    }
}
