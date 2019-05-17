package ru.glitchless.games.tprunner.ui;

import javax.swing.*;
import java.awt.*;

public class SplashPanel extends JPanel {
    /**
     * The splash image
     */
    private Image image;

    /**
     * Basic constructor
     *
     * @param image The splash image
     */
    public SplashPanel(Image image) {
        super(new BorderLayout());
        this.image = image;
    }

    @Override
    public void paintComponent(Graphics g) {
        super.paintComponent(g);
        g.drawImage(image, 0, 0, this.getWidth(), this.getHeight(), this);
    }

}
