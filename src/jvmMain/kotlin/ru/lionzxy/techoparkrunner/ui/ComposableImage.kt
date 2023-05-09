package ru.lionzxy.techoparkrunner.ui

import androidx.compose.foundation.Image
import androidx.compose.foundation.LocalIndication
import androidx.compose.foundation.clickable
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.layout.*
import androidx.compose.material.Icon
import androidx.compose.material.ripple.rememberRipple
import androidx.compose.runtime.Composable
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.unit.dp

@Composable
fun ComposableImage(
    onClose: () -> Unit
) {
    Box {
        Image(
            modifier = Modifier.fillMaxWidth(),
            painter = painterResource("background.png"),
            contentDescription = null,
            contentScale = ContentScale.FillWidth
        )
        Box(
            Modifier.align(Alignment.TopEnd)
                .clickable(
                    onClick = onClose,
                    indication = rememberRipple(),
                    interactionSource = remember { MutableInteractionSource() }
                )
        ) {
            Icon(
                modifier = Modifier.padding(horizontal = 16.dp, vertical = 14.dp)
                    .size(12.dp),
                painter = painterResource("ic_close.svg"),
                contentDescription = "Close",
                tint = Color.White
            )
        }
    }
}