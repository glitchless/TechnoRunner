package ru.lionzxy.techoparkrunner.ui

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.LinearProgressIndicator
import androidx.compose.material.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontStyle
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.platform.Font
import androidx.compose.ui.unit.dp
import ru.lionzxy.techoparkrunner.model.ProgressState

private val fontFamily = FontFamily(
    listOf(
        Font(
            resource = "roboto.ttf",
            weight = FontWeight.W400,
            style = FontStyle.Normal
        )
    )
)

@Composable
fun ComposableProgressBar(progressState: ProgressState) {
    Column(
        modifier = Modifier.height(70.dp)
            .fillMaxWidth()
            .background(Color(0xFF2F3136)),
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.Center
    ) {
        Text(
            modifier = Modifier.padding(bottom = 10.dp),
            text = progressState.text,
            fontFamily = fontFamily,
            color = Color(0xFFDDDDDE)
        )
        ComposableProgressIndicator(progressState.progress)
    }
}

@Composable
private fun ComposableProgressIndicator(progress: Float?) {
    val progressModifier = Modifier.fillMaxWidth()
        .padding(horizontal = 22.dp)
        .height(12.dp)
        .clip(RoundedCornerShape(2.dp))
    if (progress == null) {
        LinearProgressIndicator(
            modifier = progressModifier,
            strokeCap = StrokeCap.Square,
            backgroundColor = Color(0xFFDDDDDE),
            color = Color(0xFF00DB9D)
        )
    } else {
        LinearProgressIndicator(
            modifier = progressModifier,
            strokeCap = StrokeCap.Square,
            backgroundColor = Color(0xFFDDDDDE),
            color = Color(0xFF00DB9D),
            progress = progress
        )
    }
}