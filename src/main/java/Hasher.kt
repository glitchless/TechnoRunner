import utils.Utils
import java.io.File

fun main() {
    println(Utils.generateSHA256(File("TechnoparkLauncher-1.0-SNAPSHOT.jar")))
}