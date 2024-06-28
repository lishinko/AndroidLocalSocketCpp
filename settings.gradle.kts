pluginManagement {
    repositories {
        google()
        maven{ url = uri("https://maven.aliyun.com/repository/public")}
        maven{ url = uri("https://maven.aliyun.com/repository/google")}
        mavenCentral()
        gradlePluginPortal()
    }
}
dependencyResolutionManagement {
    repositoriesMode.set(RepositoriesMode.FAIL_ON_PROJECT_REPOS)
    repositories {
        google()
        maven{ url = uri("https://maven.aliyun.com/repository/public")}
        maven{ url = uri("https://maven.aliyun.com/repository/google")}
        mavenCentral()
    }
}

rootProject.name = "TestAndroidCPP"
include(":app")
 