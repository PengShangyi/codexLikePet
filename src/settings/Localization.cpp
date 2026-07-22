#include "settings/Localization.h"

#include <QLocale>

Localization::Localization(AppSettings *settings, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
{
    connect(settings, &AppSettings::languageChanged, this, [this] { emit languageChanged(); });
}

bool Localization::usesChinese() const
{
    if (m_settings->language() == AppLanguage::SimplifiedChinese) return true;
    if (m_settings->language() == AppLanguage::English) return false;
    return QLocale::system().language() == QLocale::Chinese;
}

QString Localization::text(TextKey key) const
{
    const bool zh = usesChinese();
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
    }
    return {};
}
