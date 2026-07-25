#include "settings/Localization.h"

#include <QLocale>

Localization::Localization(AppSettings *settings, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
{
    refreshLanguage();
    connect(settings, &AppSettings::languageChanged, this, [this] {
        refreshLanguage();  // before re-emitting, so slots read the new language
        emit languageChanged();
    });
}

void Localization::refreshLanguage()
{
    const AppLanguage language = m_settings->language();
    m_usesChinese = language == AppLanguage::SimplifiedChinese
        || (language == AppLanguage::System
            && QLocale::system().language() == QLocale::Chinese);
}

bool Localization::usesChinese() const
{
    return m_usesChinese;
}

QString Localization::text(TextKey key) const
{
    const bool zh = m_usesChinese;
    switch (key) {
    case TextKey::ShowPet: return zh ? QStringLiteral("显示宠物") : QStringLiteral("Show Pet");
    case TextKey::HidePet: return zh ? QStringLiteral("隐藏宠物") : QStringLiteral("Hide Pet");
    case TextKey::Settings: return zh ? QStringLiteral("设置…") : QStringLiteral("Settings…");
    case TextKey::Quit: return zh ? QStringLiteral("退出 Potato") : QStringLiteral("Quit Potato");
    case TextKey::General: return zh ? QStringLiteral("通用") : QStringLiteral("General");
    case TextKey::Pet: return zh ? QStringLiteral("宠物") : QStringLiteral("Pet");
    case TextKey::ImportPet: return zh ? QStringLiteral("导入宠物…") : QStringLiteral("Import Pet…");
    case TextKey::RemovePet: return zh ? QStringLiteral("删除宠物") : QStringLiteral("Remove Pet");
    case TextKey::Size: return zh ? QStringLiteral("尺寸") : QStringLiteral("Size");
    case TextKey::AnimationSpeed: return zh ? QStringLiteral("动画速度") : QStringLiteral("Animation speed");
    case TextKey::AlwaysOnTop: return zh ? QStringLiteral("始终置顶") : QStringLiteral("Always on top");
    case TextKey::LaunchAtLogin: return zh ? QStringLiteral("登录时启动") : QStringLiteral("Launch at login");
    case TextKey::TypingDetection: return zh ? QStringLiteral("打字活动动画") : QStringLiteral("Typing activity animation");
    case TextKey::TypingPrivacyNote: return zh ? QStringLiteral("只统计活动时间，不读取或保存按键内容。") : QStringLiteral("Records activity timing only; key contents are never read or stored.");
    case TextKey::ReducedMotion: return zh ? QStringLiteral("动态效果") : QStringLiteral("Motion");
    case TextKey::FollowSystem: return zh ? QStringLiteral("跟随系统") : QStringLiteral("Follow system");
    case TextKey::ReduceMotion: return zh ? QStringLiteral("减少动态") : QStringLiteral("Reduce motion");
    case TextKey::FullMotion: return zh ? QStringLiteral("完整动态") : QStringLiteral("Full motion");
    case TextKey::Hemisphere: return zh ? QStringLiteral("季节半球") : QStringLiteral("Season hemisphere");
    case TextKey::North: return zh ? QStringLiteral("北半球") : QStringLiteral("Northern");
    case TextKey::South: return zh ? QStringLiteral("南半球") : QStringLiteral("Southern");
    case TextKey::DayStarts: return zh ? QStringLiteral("白天开始") : QStringLiteral("Day starts");
    case TextKey::NightStarts: return zh ? QStringLiteral("夜间开始") : QStringLiteral("Night starts");
    case TextKey::Language: return zh ? QStringLiteral("界面语言") : QStringLiteral("Language");
    case TextKey::SystemLanguage: return zh ? QStringLiteral("跟随系统") : QStringLiteral("System default");
    case TextKey::English: return QStringLiteral("English");
    case TextKey::SimplifiedChinese: return QStringLiteral("简体中文");
    case TextKey::ResetPosition: return zh ? QStringLiteral("重置宠物位置") : QStringLiteral("Reset pet position");
    case TextKey::Close: return zh ? QStringLiteral("关闭") : QStringLiteral("Close");
    case TextKey::Preview: return zh ? QStringLiteral("预览") : QStringLiteral("Preview");
    case TextKey::NoPetSelected: return zh ? QStringLiteral("暂无可用宠物") : QStringLiteral("No pet available");
    case TextKey::ImportPackage: return zh ? QStringLiteral("导入 .potatopet…") : QStringLiteral("Import .potatopet…");
    case TextKey::ImportDirectory: return zh ? QStringLiteral("导入资源目录…") : QStringLiteral("Import resource directory…");
    case TextKey::ImportSucceeded: return zh ? QStringLiteral("宠物导入成功") : QStringLiteral("Pet imported successfully");
    case TextKey::ImportFailed: return zh ? QStringLiteral("宠物导入失败") : QStringLiteral("Pet import failed");
    case TextKey::RemoveConfirmation: return zh ? QStringLiteral("确定删除这个本地宠物吗？") : QStringLiteral("Remove this local pet?");
    case TextKey::ValidationReport: return zh ? QStringLiteral("校验报告") : QStringLiteral("Validation report");
    case TextKey::InputPermissionTitle: return zh ? QStringLiteral("需要输入监控权限") : QStringLiteral("Input Monitoring required");
    case TextKey::InputPermissionBody: return zh ? QStringLiteral("Potato 只统计按键活动时间，不读取或保存按键内容。请在系统设置中允许输入监控后重新开启此选项。") : QStringLiteral("Potato records activity timing only and never reads or stores key contents. Allow Input Monitoring in System Settings, then enable this option again.");
    case TextKey::OpenSystemSettings: return zh ? QStringLiteral("打开系统设置") : QStringLiteral("Open System Settings");
    case TextKey::LoginItemErrorTitle: return zh ? QStringLiteral("无法更新登录项") : QStringLiteral("Unable to update login item");
    case TextKey::PreviewVariant: return zh ? QStringLiteral("预览图集") : QStringLiteral("Preview atlas");
    case TextKey::PreviewAnimation: return zh ? QStringLiteral("预览动作") : QStringLiteral("Preview animation");
    case TextKey::PreviewClip: return zh ? QStringLiteral("扩展动画条") : QStringLiteral("Extension clip");
    case TextKey::ResourceFallbacks: return zh ? QStringLiteral("资源与回退") : QStringLiteral("Resources and fallbacks");
    case TextKey::PetLoadErrorTitle: return zh ? QStringLiteral("无法加载宠物") : QStringLiteral("Couldn't load pet");
    case TextKey::StartupFailedTitle: return zh ? QStringLiteral("Potato 无法启动") : QStringLiteral("Potato can't start");
    case TextKey::StartupFailedBody: return zh ? QStringLiteral("无法创建菜单栏图标。Potato 需要可用的系统菜单栏才能运行。") : QStringLiteral("The menu bar item couldn't be created. Potato needs an available system menu bar to run.");
    case TextKey::OnboardingTitle: return zh ? QStringLiteral("欢迎使用 Potato") : QStringLiteral("Welcome to Potato");
    case TextKey::OnboardingIntro: return zh ? QStringLiteral("Potato 是一只安静地待在桌面上的小宠物。几点须知：") : QStringLiteral("Potato is a little desktop companion that quietly lives on your screen. A few things to know:");
    case TextKey::OnboardingMenuBarHeading: return zh ? QStringLiteral("它住在菜单栏") : QStringLiteral("It lives in the menu bar");
    case TextKey::OnboardingMenuBarBody: return zh ? QStringLiteral("Potato 没有程序坞图标。点按屏幕右上角菜单栏里的土豆图标即可打开菜单。") : QStringLiteral("Potato has no Dock icon. Click the potato in the menu bar at the top-right of your screen to open its menu.");
    case TextKey::OnboardingSettingsHeading: return zh ? QStringLiteral("在设置里调整") : QStringLiteral("Adjust it in Settings");
    case TextKey::OnboardingSettingsBody: return zh ? QStringLiteral("从菜单栏打开“设置…”可以调整大小、动画速度、季节半球与语言。") : QStringLiteral("Open “Settings…” from the menu bar to change size, animation speed, season hemisphere, and language.");
    case TextKey::OnboardingTypingHeading: return zh ? QStringLiteral("打字动画是可选的") : QStringLiteral("Typing animation is optional");
    case TextKey::OnboardingTypingBody: return zh ? QStringLiteral("默认关闭。开启后只统计按键活动时间，绝不读取或保存按键内容。") : QStringLiteral("Off by default. When enabled it only measures activity timing and never reads or stores key contents.");
    case TextKey::OnboardingImportHeading: return zh ? QStringLiteral("可以导入更多宠物") : QStringLiteral("Import more pets");
    case TextKey::OnboardingImportBody: return zh ? QStringLiteral("在“设置 › 宠物”里导入 .potatopet 或资源目录，即可添加自定义宠物。") : QStringLiteral("Add custom pets by importing a .potatopet file or resource directory under “Settings › Pet”.");
    case TextKey::OnboardingGetStarted: return zh ? QStringLiteral("开始使用") : QStringLiteral("Get started");
    case TextKey::AboutMenuItem: return zh ? QStringLiteral("关于 Potato…") : QStringLiteral("About Potato…");
    case TextKey::WelcomeMenuItem: return zh ? QStringLiteral("欢迎向导…") : QStringLiteral("Welcome Guide…");
    case TextKey::AboutTagline: return zh ? QStringLiteral("本地优先的 macOS 菜单栏桌面宠物。") : QStringLiteral("A local-first macOS menu-bar desktop pet.");
    case TextKey::AboutCopyright: return zh ? QStringLiteral("© 2026 Potato。保留所有权利。") : QStringLiteral("© 2026 Potato. All rights reserved.");
    case TextKey::AboutVersionLabel: return zh ? QStringLiteral("版本") : QStringLiteral("Version");
    case TextKey::AboutViewLicenses: return zh ? QStringLiteral("第三方许可…") : QStringLiteral("Third-party licenses…");
    case TextKey::PetAccessibleName: return zh ? QStringLiteral("Potato 桌面宠物") : QStringLiteral("Potato desktop pet");
    case TextKey::PetAccessibleDescription: return zh ? QStringLiteral("可拖动的桌面宠物，点击可与它互动。") : QStringLiteral("A draggable desktop pet. Click it to interact.");
    }
    return {};
}
