#include <jni.h>
#include <string>
#include <android/log.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define LOG_TAG "LocalSocketServer"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
static const char SOCKET_NAME[] = "\0com.saturnv.hmi.sr.socket";

extern "C" JNIEXPORT jstring
JNICALL
Java_com_example_testandroidcpp_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}
//注意点：1，不考虑字符串结尾的\0，2，bind和sendto中的地址长度，应该是真实地址长度，不能设置为sizeof(server_addr)，
// 此2点问题参考下面的链接：
//https://stackoverflow.com/questions/14643571/localsocket-communication-with-unix-domain-in-android-ndk
static int GetLengthOfAbstractSocketName(){

}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_testandroidcpp_MainActivity_testLocalSocketServer(JNIEnv *env, jobject thiz) {
    // TODO: implement testLocalSocketServer()
    LOGD("before startsocket server");
    int server_sock, client_sock;
    struct sockaddr_un server_addr, client_addr;
    socklen_t client_addr_len;

//    server_sock = socket(AF_LOCAL, SOCK_STREAM, 0);
    server_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_sock < 0) {
        LOGD("Failed to create socket");
        return;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    memcpy(server_addr.sun_path, SOCKET_NAME, sizeof(SOCKET_NAME)-1);
//    server_addr.sun_path[0] = '\0';
//    strcpy(server_addr.sun_path+1, SOCKET_NAME);
//    const char* nameStart = server_addr.sun_path + 1;
//    LOGD("name = %s", nameStart);
//    strcpy(server_addr.sun_path, SOCKET_NAME);

    unlink(SOCKET_NAME);
    const int addressLength = sizeof(server_addr.sun_family) + sizeof(SOCKET_NAME) - 1;
    if (bind(server_sock, (struct sockaddr *)&server_addr, addressLength) < 0) {
        LOGD("Failed to bind socket");
        shutdown(server_sock, SHUT_RDWR);
        close(server_sock);
        return;
    }

    if (listen(server_sock, 5) < 0) {
        LOGD("Failed to listen on socket");
        shutdown(server_sock, SHUT_RDWR);
        close(server_sock);
        return;
    }

    LOGD("Server is listening...");

    client_addr_len = sizeof(client_addr);
    client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_len);
    if (client_sock < 0) {
        LOGD("Failed to accept client connection");
        close(server_sock);
        return;
    }

    LOGD("Client connected");

    char buffer[256];
    ssize_t n;

    while ((n = read(client_sock, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[n] = '\0';
        LOGD("Received message: %s", buffer);

        // Echo the message back to the client
//        write(client_sock, buffer, n);
    }

    close(client_sock);
    shutdown(server_sock, SHUT_RDWR);
    close(server_sock);
    LOGD("Server shut down");

}

