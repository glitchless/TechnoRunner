import utils.Utils
import java.io.File

fun main(args: Array<String>) {
    println(Utils.generateSHA256(File(args[0])))
}