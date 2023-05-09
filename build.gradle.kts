import org.jetbrains.compose.desktop.application.dsl.TargetFormat

plugins {
    kotlin("multiplatform")
    kotlin("plugin.serialization")
    id("org.jetbrains.compose")
}

group = "ru.lionzxy"
version = "1.0-SNAPSHOT"

repositories {
    google()
    mavenCentral()
    maven("https://jitpack.io")
    maven("https://maven.pkg.jetbrains.space/public/p/compose/dev")
}

kotlin {
    jvm {
        jvmToolchain(11)
        withJava()
    }
    sourceSets {
        val jvmMain by getting {
            dependencies {
                implementation(compose.desktop.currentOs)
                implementation("org.jetbrains.kotlinx:kotlinx-serialization-json:${extra["serialization.version"]}")
                implementation("io.ktor:ktor-client-core:${extra["ktor.version"]}")
                implementation("io.ktor:ktor-client-okhttp:${extra["ktor.version"]}")
                implementation("io.ktor:ktor-client-content-negotiation:${extra["ktor.version"]}")
                implementation("io.ktor:ktor-serialization-kotlinx-json:${extra["ktor.version"]}")
                implementation("com.github.LionZXY:oslib:d5ba9facde")
                implementation("com.squareup.okio:okio:3.3.0")
                implementation("org.apache.commons:commons-compress:1.23.0")
            }
        }
        val jvmTest by getting

    }
}

compose.desktop {
    application {
        mainClass = "ru.lionzxy.techoparkrunner.MainKt"
        nativeDistributions {
            targetFormats(TargetFormat.Dmg, TargetFormat.Msi, TargetFormat.Deb)
            packageName = "KMMTechnoparkRunner"
            packageVersion = "1.0.0"
        }
    }
}
