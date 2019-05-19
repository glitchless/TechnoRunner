package ru.glitchless.games.tprunner.ui

import java.awt.Dimension
import java.awt.Graphics
import java.awt.Graphics2D
import java.awt.RenderingHints
import javax.swing.JComponent
import javax.swing.JProgressBar
import javax.swing.plaf.basic.BasicProgressBarUI

class GProgressBar : JProgressBar() {
    init {
        setUI(GProgressUI())
    }

    private class GProgressUI : BasicProgressBarUI() {
        override fun getPreferredInnerHorizontal(): Dimension {
            return Dimension(1, 20)
        }

        override fun paintDeterminate(g: Graphics, c: JComponent) {
            assert(progressBar.orientation == JProgressBar.HORIZONTAL)

            val g2d = g as Graphics2D
            g2d.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON)

            val b = progressBar.insets
            val barRectWidth = progressBar.width - (b.right + b.left)
            val barRectHeight = progressBar.height - (b.top + b.bottom)
            val barRectFilledWidth = getAmountFull(b, barRectWidth, barRectHeight)

            g.setColor(progressBar.background)
            g.fillRoundRect(b.left, b.top, barRectWidth, barRectHeight, 4, 4)
            g.setColor(progressBar.foreground)
            g.fillRoundRect(b.left, b.top, barRectFilledWidth, barRectHeight, 4, 4)
        }
    }
}
