package ru.glitchless.games.tprunner.ui

import sk.tomsik68.mclauncher.api.ui.IProgressMonitor
import java.awt.*
import java.awt.event.MouseAdapter
import java.awt.event.MouseEvent
import javax.imageio.ImageIO
import javax.swing.*

class SplashScreen : JFrame(), IProgressMonitor {
    private val progressBar: JProgressBar
    private val label: JLabel
    private var current = 0

    init {
        val backgroundPanel = makeBackgroundPanel()
        contentPane.add(backgroundPanel, BorderLayout.NORTH)

        val (statusPanel, progressBar, label) = makeStatusPanel()
        this.progressBar = progressBar
        this.label = label
        contentPane.add(statusPanel, BorderLayout.SOUTH)

        prepareWindow()
    }

    private fun makeBackgroundPanel(): JPanel {
        val root = JPanel()
        root.layout = OverlayLayout(root)

        val closeBtnPanel = JPanel()
        closeBtnPanel.layout = null
        closeBtnPanel.isOpaque = false
        root.add(closeBtnPanel)

        val closeBtnImage = ImageIO.read(javaClass.getResource("/close-btn.png"))
        val closeBtnIcon = ImageIcon(closeBtnImage)
        val closeBtn = JLabel(closeBtnIcon)
        closeBtn.setSize(closeBtnImage.width, closeBtnImage.height)
        closeBtn.cursor = Cursor.getPredefinedCursor(Cursor.HAND_CURSOR)
        closeBtn.addMouseListener(object : MouseAdapter() {
            override fun mouseClicked(e: MouseEvent) {
                System.exit(0)
            }
        })
        closeBtnPanel.add(closeBtn)

        val backgroundImage = ImageIO.read(javaClass.getResource("/background.jpg"))
        val backgroundIcon = ImageIcon(backgroundImage)
        val background = JLabel(backgroundIcon)
        root.add(background)

        closeBtn.setLocation(backgroundImage.width - closeBtnImage.width - 25, 25)

        return root
    }

    private fun makeStatusPanel(): Triple<JPanel, JProgressBar, JLabel> {
        val root = JPanel()
        root.layout = BorderLayout()
        root.background = Color(0x303135)

        val label = JLabel()
        label.text = "Загрузка..."
        label.foreground = Color(0xddddde)
        label.horizontalAlignment = SwingConstants.CENTER
        label.border = BorderFactory.createEmptyBorder(30, 40, 15, 40)
        val availFontNames = GraphicsEnvironment.getLocalGraphicsEnvironment().availableFontFamilyNames
        val fontName = listOf("Helvetica", "Arial").find { availFontNames.contains(it) }
        label.font = Font(fontName, Font.PLAIN, 20)
        root.add(label, BorderLayout.NORTH)

        val progressBar = GProgressBar()
        progressBar.border = BorderFactory.createEmptyBorder(0, 40, 30, 40)
        progressBar.background = Color(0xddddde)
        progressBar.foreground = Color(0x00db9d)
        root.add(progressBar, BorderLayout.SOUTH)

        return Triple(root, progressBar, label)
    }

    private fun prepareWindow() {
        title = "Загрузка..."
        isUndecorated = true
        defaultCloseOperation = WindowConstants.EXIT_ON_CLOSE
        pack()
        setLocationRelativeTo(null)
    }

    /**
     * Displays the splash (same as setVisible(true))
     */
    fun display() {
        this.isVisible = true
    }

    /**
     * Hide the splash (same as setVisible(false))
     */
    fun stop() {
        this.isVisible = false
    }

    override fun setProgress(progress: Int) {
        current = progress
        progressBar.value = progress
    }

    override fun setMax(len: Int) {
        progressBar.maximum = len
    }

    override fun incrementProgress(amount: Int) {
        progressBar.value = current + amount
    }

    override fun setStatus(status: String) {
        label.text = status
    }
}
