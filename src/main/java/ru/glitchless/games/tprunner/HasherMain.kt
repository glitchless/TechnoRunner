package ru.glitchless.games.tprunner

import ru.glitchless.games.tprunner.utils.HashUtils.generateSHA256
import java.io.File

fun main() {
    println(generateSHA256(File("TechnoparkLauncher-1.0-SNAPSHOT.jar")))
}