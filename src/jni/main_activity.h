#pragma once

#include <fake-jni/fake-jni.h>
#include "java_types.h"
#include "../text_input_handler.h"

class BuildVersion : public FakeJni::JObject {
public:
    DEFINE_CLASS_NAME("android/os/Build$VERSION")

    static FakeJni::JInt SDK_INT;
    static std::shared_ptr<FakeJni::JString> RELEASE;
};

class PackageInfo : public FakeJni::JObject {
public:
    DEFINE_CLASS_NAME("android/content/pm/PackageInfo")

    PackageInfo() {
        versionName = std::make_shared<FakeJni::JString>("TODO");
    }
    std::shared_ptr<FakeJni::JString> versionName;
};

class PackageManager : public FakeJni::JObject {
public:
    DEFINE_CLASS_NAME("android/content/pm/PackageManager")

    std::shared_ptr<PackageInfo> getPackageInfo(std::shared_ptr<FakeJni::JString> packageName, FakeJni::JInt flags) {
        return std::make_shared<PackageInfo>(PackageInfo());
    }
};

class Context : public FakeJni::JObject {
public:
    DEFINE_CLASS_NAME("android/content/Context")

    virtual std::shared_ptr<File> getFilesDir() = 0;

    virtual std::shared_ptr<File> getCacheDir() = 0;

    std::shared_ptr<ClassLoader> getClassLoader() {
        return ClassLoader::getInstance();
    }

    std::shared_ptr<Context> getApplicationContext() {
        return std::static_pointer_cast<Context>(shared_from_this());
    }

    std::shared_ptr<FakeJni::JString> getPackageName() {
        return std::make_shared<FakeJni::JString>("com.mojang.minecraftpe");
    }

    std::shared_ptr<PackageManager> getPackageManager() {
        return std::make_shared<PackageManager>(PackageManager());
    }
};

class ContextWrapper : public Context {
public:
    DEFINE_CLASS_NAME("android/content/ContextWrapper", Context)
};

class Activity : public ContextWrapper {
public:
    DEFINE_CLASS_NAME("android/app/Activity", ContextWrapper)
};

class NativeActivity : public Activity {
public:
    DEFINE_CLASS_NAME("android/app/NativeActivity", Activity)
};

class NetworkMonitor : public FakeJni::JObject {
public:
    DEFINE_CLASS_NAME("com/mojang/minecraftpe/NetworkMonitor")
};

class HardwareInfo : public FakeJni::JObject {
public:
    DEFINE_CLASS_NAME("com/mojang/minecraftpe/HardwareInformation")

    static std::shared_ptr<FakeJni::JString> getAndroidVersion() {
#ifdef __APPLE__
        return std::make_shared<FakeJni::JString>("macOS");
#else
        return std::make_shared<FakeJni::JString>("Linux");
#endif
    }

    std::shared_ptr<FakeJni::JString> getInstallerPackageName() {
        return std::make_shared<FakeJni::JString>("com.mojang.minecraftpe");
    }
};
#include <fstream>
class MainActivity : public NativeActivity {
private:
    bool ignoreNextHideKeyboard = false;
    FakeJni::JInt lastChar = 0;

public:
    unsigned char *(*stbi_load_from_memory)(unsigned char const *buffer, int len, int *x, int *y, int *channels_in_file, int desired_channels);
    void (*stbi_image_free)(void *retval_from_stbi_load);

    DEFINE_CLASS_NAME("com/mojang/minecraftpe/MainActivity", NativeActivity)

    std::string storageDirectory;
    TextInputHandler *textInput = nullptr;
    std::function<void()> quitCallback;
    GameWindow *window;

    int getAndroidVersion() {
        return BuildVersion::SDK_INT;
    }

    int getScreenWidth() {
        int width, height;
        window->getWindowSize(width, height);
        return width;
    }

    int getScreenHeight() {
        int width, height;
        window->getWindowSize(width, height);
        return height;
    }

    int getDisplayWidth() {
        int width, height;
        window->getWindowSize(width, height);
        return width;
    }

    int getDisplayHeight() {
        int width, height;
        window->getWindowSize(width, height);
        return height;
    }

    void tick() {}

    FakeJni::JBoolean isNetworkEnabled(FakeJni::JBoolean wifi) {
        return true;
    }

    FakeJni::JBoolean isChromebook() {
        return true;
    }

    std::shared_ptr<FakeJni::JString> getLocale() {
        return std::make_shared<FakeJni::JString>("en");
    }

    std::shared_ptr<FakeJni::JString> getDeviceModel() {
#ifdef __APPLE__
        return std::make_shared<FakeJni::JString>("macOS");
#else
        return std::make_shared<FakeJni::JString>("Linux");
#endif
    }

    std::shared_ptr<File> getFilesDir() override {
        return std::make_shared<File>(storageDirectory);
    }

    std::shared_ptr<File> getCacheDir() override {
        return std::make_shared<File>(storageDirectory);
    }

    std::shared_ptr<FakeJni::JString> getExternalStoragePath() {
        return std::make_shared<FakeJni::JString>(storageDirectory);
    }

    std::shared_ptr<FakeJni::JString> getInternalStoragePath() {
        return getExternalStoragePath();
    }

    std::shared_ptr<FakeJni::JString> getLegacyExternalStoragePath(std::shared_ptr<FakeJni::JString> gameFolder) {
        return std::make_shared<FakeJni::JString>("");
    }

    FakeJni::JBoolean hasWriteExternalStoragePermission() {
        return true;
    }

    std::shared_ptr<HardwareInfo> getHardwareInfo() {
        return std::make_shared<HardwareInfo>();
    }

    FakeJni::JFloat getPixelsPerMillimeter();

    FakeJni::JInt getPlatformDpi();

    std::shared_ptr<FakeJni::JString> createUUID();

    std::shared_ptr<FakeJni::JByteArray> getFileDataBytes(std::shared_ptr<FakeJni::JString> path);

    std::shared_ptr<FakeJni::JArray<FakeJni::JString>> getIPAddresses() {
        return std::make_shared<FakeJni::JArray<FakeJni::JString>>();
    }

    void runNativeCallbackOnUiThread(FakeJni::JLong h) {
        auto method = getClass().getMethod("(J)V", "nativeRunNativeCallbackOnUiThread");
        FakeJni::LocalFrame frame;
        method->invoke(frame.getJniEnv(), this, h);
    }

    void requestIntegrityToken(std::shared_ptr<FakeJni::JString>);

    void launchUri(std::shared_ptr<FakeJni::JString>);
    void share(std::shared_ptr<FakeJni::JString>, std::shared_ptr<FakeJni::JString>, std::shared_ptr<FakeJni::JString>);
    void shareFile(std::shared_ptr<FakeJni::JString>, std::shared_ptr<FakeJni::JString>, std::shared_ptr<FakeJni::JString>);
    std::shared_ptr<FakeJni::JArray<FakeJni::JString>> getBroadcastAddresses() {
        return std::make_shared<FakeJni::JArray<FakeJni::JString>>();
    }

    void showKeyboard(std::shared_ptr<FakeJni::JString> text, FakeJni::JInt maxLen, FakeJni::JBoolean ignored,
                      FakeJni::JBoolean ignored2, FakeJni::JBoolean multiline) {
        ignoreNextHideKeyboard = false;
        if(textInput)
            textInput->enable(text->asStdString(), multiline);
    }

    void hideKeyboard() {
        if(ignoreNextHideKeyboard) {
            ignoreNextHideKeyboard = false;
            return;
        }
        if(textInput)
            textInput->disable();
    }
    FakeJni::JBoolean hasHardwareKeyboard() {
        return true;
    }
    void updateTextboxText(std::shared_ptr<FakeJni::JString> newText) {
        if(textInput)
            textInput->update(newText->asStdString());
        ignoreNextHideKeyboard = true;
    }

    void setTextBoxBackend(std::shared_ptr<FakeJni::JString> newText) {
        if(textInput)
            textInput->update(newText->asStdString());
    }

    FakeJni::JInt getCursorPosition() {
        ignoreNextHideKeyboard = false;
        if(textInput)
            return textInput->getCursorPosition();
        return -1;
    }

    std::shared_ptr<FakeJni::JString> getTextBoxBackend() {
        if(textInput)
            return std::make_shared<FakeJni::JString>(textInput->getText());
        return std::make_shared<FakeJni::JString>("");
    }

    FakeJni::JInt getKeyFromKeyCode(FakeJni::JInt keyCode, FakeJni::JInt metaState, FakeJni::JInt deviceId);

    void setCaretPosition(FakeJni::JInt pos);

    FakeJni::JLong calculateAvailableDiskFreeSpace(std::shared_ptr<FakeJni::JString> str) {
        return 1024LL * 1024LL * 1024LL * 1024LL;
    }

    FakeJni::JLong getUsableSpace(std::shared_ptr<FakeJni::JString> str) {
        return 1024LL * 1024LL * 1024LL * 1024LL;
    }

    FakeJni::JInt getCaretPosition();

    void lockCursor();

    void unlockCursor();

    FakeJni::JLong getUsedMemory();

    FakeJni::JLong getFreeMemory();

    FakeJni::JLong getTotalMemory();

    FakeJni::JLong getMemoryLimit();

    FakeJni::JLong getAvailableMemory();

    void pickImage(FakeJni::JLong callback);

    void setClipboard(std::shared_ptr<FakeJni::JString>);

    void initializeXboxLive(FakeJni::JLong xalinit, FakeJni::JLong xblinit);

    FakeJni::JLong initializeXboxLive2(FakeJni::JLong xalinit, FakeJni::JLong xblinit);

    FakeJni::JLong initializeLibHttpClient(FakeJni::JLong init);

    std::shared_ptr<FakeJni::JIntArray> getImageData(std::shared_ptr<FakeJni::JString> filename);

    void startPlayIntegrityCheck();

    void openFile();
    void saveFile(std::shared_ptr<FakeJni::JString>);

    void setLastChar(FakeJni::JInt sym);

    FakeJni::JLong getAllocatableBytes(std::shared_ptr<FakeJni::JString> path);
    FakeJni::JBoolean supportsSizeQuery(std::shared_ptr<FakeJni::JString> path);
};

class JellyBeanDeviceManager : public FakeJni::JObject {
public:
    DEFINE_CLASS_NAME("com/mojang/minecraftpe/input/JellyBeanDeviceManager")
};

class PlayIntegrity : public FakeJni::JObject {
public:
    DEFINE_CLASS_NAME("com/mojang/minecraftpe/PlayIntegrity")
};
