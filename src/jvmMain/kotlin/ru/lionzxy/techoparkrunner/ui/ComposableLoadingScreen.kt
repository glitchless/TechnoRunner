package ru.lionzxy.techoparkrunner.ui

import androidx.compose.foundation.layout.Column
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import ru.lionzxy.techoparkrunner.model.ProgressState

@Composable
fun ComposableLoadingScreen(
    modifier: Modifier,
    progressState: ProgressState,
    onClose: () -> Unit
) {
    Column(modifier) {
        ComposableImage(onClose)
        ComposableProgressBar(progressState)
    }
}