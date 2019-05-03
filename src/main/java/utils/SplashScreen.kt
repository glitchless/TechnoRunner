package utils

import sk.tomsik68.mclauncher.api.ui.IProgressMonitor
import java.awt.*
import javax.imageio.ImageIO
import javax.swing.*

class SplashScreen() : JFrame(), IProgressMonitor {
    private val progressBar = JProgressBar()
    private val label = JLabel()
    private var current = 0

    init {
        val image = ImageIO.read(javaClass.getResource("/minelogo.png"))

        label.verticalAlignment = JLabel.BOTTOM
        label.horizontalAlignment = JLabel.CENTER

        val font = Font.createFont(Font.TRUETYPE_FONT, javaClass.getResource("/Gugi-Regular.ttf").openStream())
        GraphicsEnvironment.getLocalGraphicsEnvironment().registerFont(font)
        label.font = font.deriveFont(16f)
        label.foreground = Color.WHITE

        progressBar.background = Color.WHITE
        progressBar.foreground = Color.GREEN

        this.title = "Loading..."
        this.isUndecorated = true
        this.defaultCloseOperation = WindowConstants.EXIT_ON_CLOSE
        this.setSize(image.getWidth(this), image.getHeight(this))
        this.setLocationRelativeTo(null)
        contentPane = SplashPanel(image)

        val panel = JPanel()
        panel.layout = BoxLayout(panel, BoxLayout.Y_AXIS)
        panel.isOpaque = false
        label.alignmentX = Component.CENTER_ALIGNMENT
        progressBar.alignmentX = Component.CENTER_ALIGNMENT
        progressBar.maximumSize = Dimension(300, Integer.MAX_VALUE)
        panel.add(label)
        panel.add(progressBar)
        contentPane.add(panel, BorderLayout.SOUTH)

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
