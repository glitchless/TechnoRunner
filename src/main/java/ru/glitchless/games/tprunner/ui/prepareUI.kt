package ru.glitchless.games.tprunner.ui

import java.awt.Color
import javax.swing.UIManager

fun prepareUI() {
    UIManager.put("ProgressBar.background", Color.WHITE)
    UIManager.put("ProgressBar.foreground", Color.GREEN)
    UIManager.put("ProgressBar.selectionBackground", Color.WHITE)
    UIManager.put("ProgressBar.selectionForeground", Color.GREEN)
}