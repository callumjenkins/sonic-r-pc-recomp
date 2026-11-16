plugins {
    id("com.android.application")
}

android {
    namespace = "org.sonicr.recomp"
    compileSdk = 36
    defaultConfig {
        applicationId = "org.sonicr.recomp"
        minSdk = 26
        targetSdk = 36
        versionCode = 1
        versionName = "0.1"
    }
    sourceSets["main"].apply {
        java.srcDir(rootProject.file(".deps/SDL/android-project/app/src/main/java"))
        jniLibs.srcDir(rootProject.file("build/native"))
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
}
