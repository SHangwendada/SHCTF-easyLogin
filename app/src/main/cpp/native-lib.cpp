#include <jni.h>
#include <string>
#include <android/log.h>
#include <vector>  // 用于存储分组数据
#include "obfusheader.h"
#include <dlfcn.h>
#include "dlfcn/local_dlfcn.h"
#include <jni.h>
#include <string>
#include <jni.h>
#include <string>
#include <link.h>
#include <dlfcn.h>
#include "elf.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "sys/mman.h"
#include "android/log.h"
#include "errno.h"

#include <string>
#include <cstring>
#include <android/log.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>

#define LOG_TAG "NativeCode"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

#include <jni.h>
#include <dlfcn.h>         // 导入 dlclose() 相关的库
#include <string>
#include <unistd.h>        // 用于 sleep()
#include <arpa/inet.h>     // 用于 sockaddr_in 结构体
#include <sys/socket.h>    // 用于 socket 函数
#include <android/log.h>   // 用于打印日志

#define LOG_TAG "NativeCode"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// 关闭库并卸载自身的函数
void unloadLibrary() {
    LOGI("Unloading library...");
    void* handle = dlopen(NULL, RTLD_NOW);  // 获取当前库的句柄
    if (handle) {
            dlclose(handle);  // 卸载库
        } else {
                LOGI("Failed to get handle for library.");
            }
    LOGI("Library unloaded.");
}

static void* check_ports(void* arg) {
    while (1) {
        struct sockaddr_in sa{};
        sa.sin_family = AF_INET;

        // 设置要检查的端口
        auto port = 23946;
        sa.sin_port = htons(port);

        // 目标 IP 地址设置为 127.0.0.1
        inet_aton("127.0.0.1", &sa.sin_addr);

        // 创建套接字
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
                LOGI("Socket creation failed.");
                sleep(3);  // 休眠 3 秒后重试
                continue;
            }

        // 尝试连接目标端口
        if (connect(sock, (struct sockaddr*)&sa, sizeof(sa)) == 0) {
                LOGI("Find! IDA detected!");

                // 关闭 socket
                close(sock);

                // 卸载当前库的内存
                unloadLibrary();

                // 终止进程
                exit(0);
            }

        // 关闭套接字
        close(sock);

        // 每 3 秒重试一次
        sleep(3);
    }
    return nullptr;
}


static void* check(void* arg) {
    while ((1)){
#ifdef __LP64__
        const char *lib_path = "/system/lib64/libc.so";
#else
        const char *lib_path = "/system/lib/libc.so";
#endif
#define CMP_COUNT 8
        const char *sym_name = "signal";

        struct local_dlfcn_handle *handle = static_cast<local_dlfcn_handle *>(local_dlopen(lib_path));

        off_t offset = local_dlsym(handle,sym_name);

        FILE *fp = fopen(lib_path,"rb");
        char file_bytes[CMP_COUNT] = {0};
        if(fp != NULL){
                fseek(fp,offset,SEEK_SET);
                fread(file_bytes,1,CMP_COUNT,fp);
                fclose(fp);
            }

        void *dl_handle = dlopen(lib_path,RTLD_NOW);
        void *sym = dlsym(dl_handle,sym_name);

        int is_hook = memcmp(file_bytes,sym,CMP_COUNT) != 0;

        local_dlclose(handle);
        dlclose(dl_handle);
        if (is_hook){
                //  LOGI("FIND!Hook!");
                exit(0);
                unloadLibrary();
            }
        sleep(1);
    }

}


static void __attribute__((constructor())) anti() {
    pthread_t tid;
    LOGI("GO!");
    if (pthread_create(&tid, NULL, check_ports, NULL) != 0) {
            perror("Failed to create thread");
            exit(EXIT_FAILURE);
        }
    if (pthread_create(&tid, NULL, check, NULL) != 0) {
            perror("Failed to create thread");
            exit(EXIT_FAILURE);
    }
}



// 辅助函数：轮转位操作（循环左移）
unsigned int rotateLeft(unsigned int value, unsigned int shift) {
    return (value << shift) | (value >> (32 - shift));
}

// 辅助函数：将字符串拆分为多个整数块
static std::vector<unsigned int> stringToChunks(const std::string &input) {
    std::vector<unsigned int> chunks;
    for (size_t i = 0; i < input.size(); i += 4) {
        unsigned int chunk = 0;
        for (size_t j = 0; j < 4 && (i + j) < input.size(); ++j) {
            chunk |= static_cast<unsigned int>(input[i + j]) << (j * 8);
        }
        chunks.push_back(chunk);
    }
    return chunks;
}

// 自定义复杂哈希算法
static std::string customHash(const std::string &input) {
    // 初始化常量与变量
    const unsigned int prime1 = 0x85ebca6b;
    const unsigned int prime2 = 0xc2b2ae35;
    unsigned int hash = 0x811c9dc5; // 偏移量

    // 将输入字符串分块处理
    std::vector<unsigned int> chunks = stringToChunks(input);

    // 多轮处理，每一轮都混合所有块的数据
    for (int round = 0; round < 3; ++round) {  // 进行三轮混合
        for (unsigned int chunk : chunks) {
            // 逐块更新哈希值，使用轮转与乘法进行混合
            hash ^= chunk;
            hash = rotateLeft(hash, 13);
            hash *= prime1;

            // 额外的扰动，使数据更分散
            hash ^= hash >> 15;
            hash *= prime2;
        }
    }

    // 处理剩余部分：再进行一轮额外的扰动
    hash ^= hash >> 13;
    hash *= prime1;
    hash ^= hash >> 16;

    // 将哈希结果转换为十六进制字符串
    char hashedString[9];

    snprintf(hashedString, sizeof(hashedString), "%08x", hash);
    return std::string(hashedString);
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_swdd_easylogin_MainActivity_getPassticket(
        JNIEnv* env,
        jobject obj /* this */) {

    // 获取 Java 类：android.provider.Settings$Secure
    jclass settingsSecureClass = env->FindClass("android/provider/Settings$Secure");

    // 获取静态方法 getString 的方法 ID
    jmethodID getStringMethod = env->GetStaticMethodID(
            settingsSecureClass, "getString", "(Landroid/content/ContentResolver;Ljava/lang/String;)Ljava/lang/String;");

    // 获取 ContentResolver 实例
    jclass contextClass = env->FindClass("android/content/Context");
    jmethodID getContentResolverMethod = env->GetMethodID(
            contextClass, "getContentResolver", "()Landroid/content/ContentResolver;");
    jobject contentResolver = env->CallObjectMethod(obj, getContentResolverMethod);

    // 获取 ANDROID_ID 常量
    jstring androidIdField = env->NewStringUTF("android_id");

    // 调用 getString 方法获取设备 ID
    jobject androidId = env->CallStaticObjectMethod(settingsSecureClass, getStringMethod, contentResolver, androidIdField);

    // 转换为 C++ 字符串
    const char* androidIdCStr = env->GetStringUTFChars((jstring)androidId, nullptr);

    // 打印设备 ID 到 Logcat
    LOGI("Device ID: %s", androidIdCStr);

    // 使用自定义复杂哈希算法对 android_id 进行哈希处理
    std::string hashedId = customHash(androidIdCStr);

    // 打印哈希后的 ID
    LOGI("Hashed Device ID: %s", hashedId.c_str());

    // 释放资源
    env->ReleaseStringUTFChars((jstring)androidId, androidIdCStr);

    // 返回哈希后的 ID 给 Java 层
    return env->NewStringUTF(hashedId.c_str());
}
