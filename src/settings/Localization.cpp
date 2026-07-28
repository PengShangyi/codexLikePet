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
    case TextKey::Appearance: return zh ? QStringLiteral("外观") : QStringLiteral("Appearance");
    case TextKey::Behavior: return zh ? QStringLiteral("行为") : QStringLiteral("Behavior");
    case TextKey::Environment: return zh ? QStringLiteral("环境") : QStringLiteral("Environment");
    case TextKey::SectionCurrentPet: return zh ? QStringLiteral("当前宠物") : QStringLiteral("Current pet");
    case TextKey::SectionDisplay: return zh ? QStringLiteral("显示") : QStringLiteral("Display");
    case TextKey::SectionWindowInteraction: return zh ? QStringLiteral("窗口交互") : QStringLiteral("Window interaction");
    case TextKey::SectionActivity: return zh ? QStringLiteral("活动感知") : QStringLiteral("Activity");
    case TextKey::SectionSeasonPhase: return zh ? QStringLiteral("季节与昼夜") : QStringLiteral("Season and time of day");
    case TextKey::SectionStartup: return zh ? QStringLiteral("启动") : QStringLiteral("Startup");
    case TextKey::SectionInterface: return zh ? QStringLiteral("界面") : QStringLiteral("Interface");
    case TextKey::SectionAbout: return zh ? QStringLiteral("关于") : QStringLiteral("About");
    case TextKey::Opacity: return zh ? QStringLiteral("不透明度") : QStringLiteral("Opacity");
    case TextKey::LockPosition: return zh ? QStringLiteral("锁定宠物位置") : QStringLiteral("Lock pet position");
    case TextKey::LockPositionNote: return zh ? QStringLiteral("锁定后无法拖动；点击互动与右键菜单仍然可用。") : QStringLiteral("Dragging is disabled. Clicking and the context menu still work.");
    case TextKey::CurrentEnvironment: return zh ? QStringLiteral("当前") : QStringLiteral("Now");
    case TextKey::ResourceDetails: return zh ? QStringLiteral("资源详情") : QStringLiteral("Resource details");
    case TextKey::UseStandardAnimation: return zh ? QStringLiteral("使用标准动作") : QStringLiteral("Use standard animation");
    case TextKey::PetGuideButton: return zh ? QStringLiteral("制作宠物…") : QStringLiteral("Make a pet…");
    case TextKey::PetGuideTitle: return zh ? QStringLiteral("制作 Potato 宠物") : QStringLiteral("Make a Potato pet");
    case TextKey::PetGuideIntro: return zh ? QStringLiteral("用 gpt-image-2 之类的图像模型生成美术素材，拼装成一张精灵图集，再从这里导入。下面的提示词模版特意保留英文——图像模型对英文提示词的还原最稳定。") : QStringLiteral("Generate the artwork with an image model such as gpt-image-2, assemble it into one sprite atlas, then import it here. The prompt templates below are English on purpose: image models follow English prompts most reliably.");
    case TextKey::PetGuideContractHeading: return zh ? QStringLiteral("Potato 需要什么") : QStringLiteral("What Potato needs");
    case TextKey::PetGuideContractBody: return zh ? QStringLiteral("一张透明的 PNG 或 WebP 图集，尺寸必须正好是 1536×2288 像素：8 列 × 11 行，每格 192×208。第 0–8 行是动作状态，第 9–10 行是 16 个朝向。每行末尾未使用的格子必须完全透明，清单中的 spriteVersionNumber 必须为 2。") : QStringLiteral("One transparent PNG or WebP atlas, exactly 1536×2288 pixels: an 8-column × 11-row grid of 192×208 cells. Rows 0–8 are animation states, rows 9–10 are the 16 look directions. Unused cells at the end of a row must be fully transparent, and the manifest must set spriteVersionNumber to 2.");
    case TextKey::PetGuideStep1Heading: return zh ? QStringLiteral("1. 生成角色形象") : QStringLiteral("1. Generate the character");
    case TextKey::PetGuideStep1Body: return zh ? QStringLiteral("先生成一张宠物静止站立的参考图。后面每一行动作都以它为基准，请保留到图集完成为止——它是让角色在每一帧都保持辨识度的关键。") : QStringLiteral("Start with one reference image of the pet standing at rest. Every animation row is generated from it, so keep it until the atlas is finished: it is what keeps the character recognisable across every frame.");
    case TextKey::PetGuideStep2Heading: return zh ? QStringLiteral("2. 生成动作行") : QStringLiteral("2. Generate the animation rows");
    case TextKey::PetGuideStep2Body: return zh ? QStringLiteral("每行单独生成一条横向长条，每次都附上第 1 步的角色图作为参考。各行与帧数：idle 6、running-right 8、running-left 8、waving 4、jumping 5、failed 8、waiting 6、running 6、review 6。最后是两行朝向，各 8 帧，从 000°（正上方）开始顺时针每 22.5° 一格。") : QStringLiteral("Generate one horizontal strip per row, attaching the step 1 character as a reference every time. Rows and frame counts: idle 6, running-right 8, running-left 8, waving 4, jumping 5, failed 8, waiting 6, running 6, review 6. Then two look rows of 8 frames each, turning clockwise in 22.5° steps from 000° (up).");
    case TextKey::PetGuideStep3Heading: return zh ? QStringLiteral("3. 拼装与校验") : QStringLiteral("3. Assemble and validate");
    case TextKey::PetGuideStep3Body: return zh ? QStringLiteral("没有哪个图像模型能一次就返回像素精确的 1536×2288 图集，所以拼装这一步要自己做：切帧、抠掉背景色、按网格摆放每一格，再校验结果。随附的 Hatch Pet 技能可以完成全过程；把它复制到 ~/.codex/skills/hatch-pet 即可配合 Codex 使用。Potato 不会替你安装。") : QStringLiteral("No image model reliably returns a pixel-exact 1536×2288 atlas in one shot, so the assembly step is yours: cut the frames, key out the background, place every cell on the grid, and validate the result. The bundled Hatch Pet skill does all of that; copy it to ~/.codex/skills/hatch-pet to use it with Codex. Potato never installs it for you.");
    case TextKey::PetGuideStep4Heading: return zh ? QStringLiteral("4. 打包与导入") : QStringLiteral("4. Package and import");
    case TextKey::PetGuideStep4Body: return zh ? QStringLiteral("把 pet.json 与 spritesheet.webp 放进同一个文件夹，然后用“导入宠物 › 导入资源目录”。要分享时，把文件夹内容打包成 zip（pet.json 位于压缩包根目录）并把扩展名改为 .potatopet。季节变体与边缘动画是可选的，写在 potato.json 里——详见创作指南。") : QStringLiteral("Put pet.json beside spritesheet.webp in one folder, then use Import Pet › Import resource directory. To share it, zip the folder's contents with pet.json at the archive root and rename the archive to .potatopet. Seasonal variants and edge animations are optional and live in potato.json — see the authoring guide.");
    case TextKey::PetGuidePromptLabel: return zh ? QStringLiteral("提示词模版 —— 粘贴到 gpt-image-2") : QStringLiteral("Prompt template — paste into gpt-image-2");
    case TextKey::PetGuideRowPromptLabel: return zh ? QStringLiteral("动作行提示词模版 —— 每次生成一行，并附上第 1 步的角色图") : QStringLiteral("Row prompt template — one row per run, with the step 1 image attached");
    case TextKey::PetGuideManifestLabel: return zh ? QStringLiteral("最简 pet.json") : QStringLiteral("Minimal pet.json");
    case TextKey::PetGuidePlaceholderNote: return zh ? QStringLiteral("发送前请替换 <name>、<style>、<description>、<state> 与 <count>；尺寸与颜色数值请原样保留。") : QStringLiteral("Replace <name>, <style>, <description>, <state>, and <count> before sending. Leave the sizes and colour values exactly as written.");
    case TextKey::PetGuideCopy: return zh ? QStringLiteral("复制") : QStringLiteral("Copy");
    case TextKey::PetGuideCopied: return zh ? QStringLiteral("已复制") : QStringLiteral("Copied");
    case TextKey::PetGuideOpenDoc: return zh ? QStringLiteral("打开创作指南") : QStringLiteral("Open authoring guide");
    case TextKey::PetGuideRevealSkill: return zh ? QStringLiteral("显示 Hatch Pet 技能") : QStringLiteral("Reveal Hatch Pet skill");

    case TextKey::AssemblerButton: return zh ? QStringLiteral("拼装图集…") : QStringLiteral("Assemble atlas…");
    case TextKey::AssemblerTitle: return zh ? QStringLiteral("拼装宠物图集") : QStringLiteral("Assemble a pet atlas");
    case TextKey::AssemblerIntro: return zh ? QStringLiteral("为每一行选择第 2 步生成的长条图。Potato 会切分每一帧、抠掉背景色、统一大小与落地基准线，拼成一张 1536×2288 的图集并直接安装。") : QStringLiteral("Choose the strip you generated for each row in step 2. Potato slices out every frame, keys out the background, gives them all one size and one ground line, then assembles the 1536×2288 atlas and installs it.");
    case TextKey::AssemblerSectionRows: return zh ? QStringLiteral("动作行") : QStringLiteral("Animation rows");
    case TextKey::AssemblerSectionKey: return zh ? QStringLiteral("背景抠除") : QStringLiteral("Background removal");
    case TextKey::AssemblerSectionPet: return zh ? QStringLiteral("宠物信息") : QStringLiteral("Pet details");
    case TextKey::AssemblerChromaKey: return zh ? QStringLiteral("背景色") : QStringLiteral("Background colour");
    case TextKey::AssemblerKeyTolerance: return zh ? QStringLiteral("容差") : QStringLiteral("Tolerance");
    case TextKey::AssemblerDespill: return zh ? QStringLiteral("清除边缘残留") : QStringLiteral("Clean up edge fringe");
    case TextKey::AssemblerDisplayName: return zh ? QStringLiteral("显示名称") : QStringLiteral("Display name");
    case TextKey::AssemblerPetId: return zh ? QStringLiteral("宠物 ID") : QStringLiteral("Pet id");
    case TextKey::AssemblerChoose: return zh ? QStringLiteral("选择…") : QStringLiteral("Choose…");
    case TextKey::AssemblerFillFromFolder: return zh ? QStringLiteral("从文件夹填充…") : QStringLiteral("Fill from folder…");
    case TextKey::AssemblerDetectKey: return zh ? QStringLiteral("自动识别") : QStringLiteral("Detect");
    case TextKey::AssemblerCompose: return zh ? QStringLiteral("生成预览") : QStringLiteral("Assemble preview");
    case TextKey::AssemblerInstall: return zh ? QStringLiteral("安装宠物") : QStringLiteral("Install pet");
    case TextKey::AssemblerChooseStripTitle: return zh ? QStringLiteral("选择动作行长条图") : QStringLiteral("Choose an animation strip");
    case TextKey::AssemblerFillFromFolderTitle: return zh ? QStringLiteral("选择存放长条图的文件夹") : QStringLiteral("Choose the folder holding your strips");
    case TextKey::AssemblerNoStrip: return zh ? QStringLiteral("未选择") : QStringLiteral("Not chosen");
    case TextKey::AssemblerScaleReadout: return zh ? QStringLiteral("缩放至 %1%") : QStringLiteral("Fitted at %1%");
    case TextKey::AssemblerFilledCount: return zh ? QStringLiteral("已匹配 %1 / %2 行") : QStringLiteral("Matched %1 of %2 rows");
    case TextKey::AssemblerReady: return zh ? QStringLiteral("图集已就绪，可以安装。") : QStringLiteral("The atlas is ready to install.");

    // Contract names first, gloss second: `idle` is what the prompt said and what the
    // file is called, so replacing it would break the connection to both.
    case TextKey::AssemblerRowIdle: return zh ? QStringLiteral("idle 待机") : QStringLiteral("idle");
    case TextKey::AssemblerRowRunningRight: return zh ? QStringLiteral("running-right 向右跑") : QStringLiteral("running-right");
    case TextKey::AssemblerRowRunningLeft: return zh ? QStringLiteral("running-left 向左跑") : QStringLiteral("running-left");
    case TextKey::AssemblerRowWaving: return zh ? QStringLiteral("waving 挥手") : QStringLiteral("waving");
    case TextKey::AssemblerRowJumping: return zh ? QStringLiteral("jumping 跳跃") : QStringLiteral("jumping");
    case TextKey::AssemblerRowFailed: return zh ? QStringLiteral("failed 失落") : QStringLiteral("failed");
    case TextKey::AssemblerRowWaiting: return zh ? QStringLiteral("waiting 等待") : QStringLiteral("waiting");
    case TextKey::AssemblerRowRunning: return zh ? QStringLiteral("running 奔跑") : QStringLiteral("running");
    case TextKey::AssemblerRowReview: return zh ? QStringLiteral("review 查看") : QStringLiteral("review");
    case TextKey::AssemblerRowLookA: return zh ? QStringLiteral("look-a 朝向 000°–157.5°") : QStringLiteral("look-a (000°–157.5°)");
    case TextKey::AssemblerRowLookB: return zh ? QStringLiteral("look-b 朝向 180°–337.5°") : QStringLiteral("look-b (180°–337.5°)");

    case TextKey::AssemblerProblemMissingRow: return zh ? QStringLiteral("%1：还没有选择长条图。") : QStringLiteral("%1: no strip chosen yet.");
    case TextKey::AssemblerProblemStripTooSmall: return zh ? QStringLiteral("%1：图片太小，放不下这一行的帧数。") : QStringLiteral("%1: the image is too small to hold this row's frames.");
    case TextKey::AssemblerProblemEmptyFrame: return zh ? QStringLiteral("%1 第 %2 帧：抠除背景后什么都不剩。请检查帧数与背景色。") : QStringLiteral("%1 frame %2: nothing left after removing the background. Check the frame count and the background colour.");
    case TextKey::AssemblerProblemOccupancy: return zh ? QStringLiteral("拼装结果不符合图集格式要求。") : QStringLiteral("The assembled atlas does not satisfy the atlas contract.");
    case TextKey::AssemblerProblemStripAspect: return zh ? QStringLiteral("%1：长宽比与这一行的帧数不匹配，可能帧数不对或叠了两行。") : QStringLiteral("%1: the proportions do not match this row's frame count — it may have the wrong number of frames, or two rows stacked.");
    case TextKey::AssemblerProblemOutlierFrame: return zh ? QStringLiteral("%1 第 %2 帧比其他帧大得多，会把整张图集缩小。") : QStringLiteral("%1 frame %2 is much larger than the others, which shrinks the whole atlas.");
    case TextKey::AssemblerProblemDoesNotFit: return zh ? QStringLiteral("%1 第 %2 帧放不进格子，已裁剪。") : QStringLiteral("%1 frame %2 did not fit its cell and was trimmed.");
    }
    return {};
}
