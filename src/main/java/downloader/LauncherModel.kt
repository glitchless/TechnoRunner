package downloader

import com.google.gson.annotations.SerializedName

data class LauncherModel(
        @SerializedName("version")
        val version: String,
        @SerializedName("downloadFullPath")
        val downloadUrl: String,
        @SerializedName("SHA-256")
        val sha256: String
)