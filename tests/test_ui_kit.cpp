#include "settings/AppSettings.h"
#include "settings/Localization.h"
#include "ui/Disclosure.h"
#include "ui/InlineBanner.h"
#include "ui/SettingsCard.h"
#include "ui/SettingsPage.h"
#include "ui/SettingsRow.h"
#include "ui/Theme.h"
#include "ui/ValueSlider.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFrame>
#include <QLabel>
#include <QScrollArea>
#include <QSignalSpy>
#include <QSlider>
#include <QTemporaryDir>
#include <QTest>
#include <QVBoxLayout>

class UiKitTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        m_temp = new QTemporaryDir;
        m_settings = new AppSettings(m_temp->filePath(QStringLiteral("settings.ini")));
        m_settings->setLanguage(AppLanguage::English);
        m_localization = new Localization(m_settings);
    }

    void cleanupTestCase()
    {
        delete m_localization;
        delete m_settings;
        delete m_temp;
    }

    // The whole point of SettingsRow: a row cannot be constructed without a
    // TextKey, so the label and the control's accessible name cannot drift apart or
    // be forgotten the way they could when QFormLayout owned the label.
    void rowLabelsAndNamesItsControlFromOneKey()
    {
        auto *combo = new QComboBox;
        SettingsRow row(TextKey::Hemisphere, combo, m_localization);

        QCOMPARE(row.labelKey(), TextKey::Hemisphere);
        QCOMPARE(row.control(), combo);
        QCOMPARE(row.labelText(), m_localization->text(TextKey::Hemisphere));
        QCOMPARE(combo->accessibleName(), m_localization->text(TextKey::Hemisphere));
        QVERIFY(!row.labelText().isEmpty());
        QVERIFY(row.labelText() != QStringLiteral(" "));
    }

    void rowDescriptionAlsoBecomesTheAccessibleDescription()
    {
        auto *check = new QCheckBox;
        SettingsRow row(TextKey::TypingDetection, check, m_localization);
        row.setDescriptionKey(TextKey::TypingPrivacyNote);

        QCOMPARE(check->accessibleDescription(), m_localization->text(TextKey::TypingPrivacyNote));

        bool foundNote = false;
        for (QLabel *label : row.findChildren<QLabel *>()) {
            foundNote |= label->text() == m_localization->text(TextKey::TypingPrivacyNote);
        }
        QVERIFY(foundNote);
    }

    void rowRetranslatesLabelDescriptionAndAccessibleName()
    {
        auto *combo = new QComboBox;
        SettingsRow row(TextKey::Language, combo, m_localization);
        row.setDescriptionKey(TextKey::TypingPrivacyNote);
        const QString english = row.labelText();

        m_settings->setLanguage(AppLanguage::SimplifiedChinese);
        row.retranslate();
        const QString chinese = row.labelText();

        QVERIFY(!chinese.isEmpty());
        QVERIFY2(chinese != english, "Language must have a distinct Chinese string");
        QCOMPARE(combo->accessibleName(), chinese);
        QCOMPARE(combo->accessibleDescription(), m_localization->text(TextKey::TypingPrivacyNote));

        m_settings->setLanguage(AppLanguage::English);
        row.retranslate();
        QCOMPARE(row.labelText(), english);
    }

    void valueOnlyRowNeedsNoControl()
    {
        SettingsRow row(TextKey::AboutVersionLabel, nullptr, m_localization);
        row.setValueText(QStringLiteral("0.1.0"));
        QCOMPARE(row.control(), nullptr);

        bool foundValue = false;
        for (QLabel *label : row.findChildren<QLabel *>()) {
            foundValue |= label->text() == QStringLiteral("0.1.0");
        }
        QVERIFY(foundValue);
    }

    void cardSeparatesRowsWithHairlinesAndRetranslatesThemAll()
    {
        SettingsCard card(TextKey::Pet, m_localization);
        auto *first = new SettingsRow(TextKey::Size, new QCheckBox, m_localization);
        auto *second = new SettingsRow(TextKey::AlwaysOnTop, new QCheckBox, m_localization);
        auto *third = new SettingsRow(TextKey::Language, new QCheckBox, m_localization);
        card.addRow(first);
        card.addRow(second);
        card.addRow(third);

        // A hairline goes between rows, never before the first one.
        int hairlines = 0;
        for (QFrame *frame : card.findChildren<QFrame *>()) {
            if (frame->objectName() == QStringLiteral("cardHairline")) ++hairlines;
        }
        QCOMPARE(hairlines, 2);

        m_settings->setLanguage(AppLanguage::SimplifiedChinese);
        card.retranslate();
        QCOMPARE(first->labelText(), m_localization->text(TextKey::Size));
        QCOMPARE(third->labelText(), m_localization->text(TextKey::Language));
        m_settings->setLanguage(AppLanguage::English);
        card.retranslate();
    }

    void valueSliderKeepsItsReadoutInSyncAndReservesWidth()
    {
        ValueSlider slider(50, 200, [](int value) {
            return QStringLiteral("%1%").arg(value);
        });
        slider.setValue(120);
        QCOMPARE(slider.value(), 120);
        QCOMPARE(slider.slider()->value(), 120);

        auto *readout = slider.findChild<QLabel *>(QStringLiteral("valueReadout"));
        QVERIFY(readout);
        QCOMPARE(readout->text(), QStringLiteral("120%"));
        // Width is reserved from the widest formatted value so the slider does not
        // shift sideways as the number grows during a drag.
        QVERIFY(readout->minimumWidth() > 0);

        QSignalSpy spy(&slider, &ValueSlider::valueChanged);
        slider.setValue(75);
        QCOMPARE(readout->text(), QStringLiteral("75%"));
        QCOMPARE(spy.count(), 1);

        // Setting the same value again neither emits nor breaks the readout.
        slider.setValue(75);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(readout->text(), QStringLiteral("75%"));
    }

    void bannerHidesWhenEmptyAndExposesSeverityForTheStylesheet()
    {
        InlineBanner banner;
        QVERIFY(!banner.isVisible());
        QCOMPARE(banner.severityName(), QStringLiteral("info"));

        banner.setMessage(QStringLiteral("imported"), InlineBanner::Severity::Info);
        QCOMPARE(banner.message(), QStringLiteral("imported"));
        QCOMPARE(banner.severityName(), QStringLiteral("info"));

        banner.setMessage(QStringLiteral("bad clip"), InlineBanner::Severity::Error);
        QCOMPARE(banner.severity(), InlineBanner::Severity::Error);
        // The stylesheet selects on this property; if it stops round-tripping
        // through the meta-object, InlineBanner[severity="error"] silently stops
        // matching and errors render in the info tint.
        QCOMPARE(banner.property("severity").toString(), QStringLiteral("error"));

        banner.clear();
        QVERIFY(banner.message().isEmpty());
        QVERIFY(!banner.isVisible());
    }

    void disclosureStartsCollapsedAndTogglesItsContent()
    {
        Disclosure disclosure(TextKey::ResourceFallbacks, m_localization);
        auto *child = new QComboBox;
        disclosure.contentLayout()->addWidget(child);
        disclosure.show();
        QVERIFY(!disclosure.isExpanded());
        QVERIFY(!child->isVisible());

        // findChildren ignores visibility, so a collapsed disclosure still holds its
        // content -- which is exactly why a test that looks for the debug combos by
        // label would keep passing after they moved in here.
        QCOMPARE(disclosure.findChildren<QComboBox *>().size(), 1);

        disclosure.setExpanded(true);
        QVERIFY(disclosure.isExpanded());
        QVERIFY(child->isVisible());

        disclosure.setExpanded(false);
        QVERIFY(!disclosure.isExpanded());
        QVERIFY(!child->isVisible());
    }

    void pageKeepsCardsTopAlignedAndRetranslatesThem()
    {
        SettingsPage page;
        auto *card = new SettingsCard(TextKey::General, m_localization);
        auto *row = new SettingsRow(TextKey::LaunchAtLogin, new QCheckBox, m_localization);
        card->addRow(row);
        page.addCard(card);

        // The scroll area must not look like one.
        auto *scroll = page.findChild<QScrollArea *>(QStringLiteral("pageScroll"));
        QVERIFY(scroll);
        QCOMPARE(scroll->frameShape(), QFrame::NoFrame);
        QVERIFY(scroll->widgetResizable());
        QVERIFY(!scroll->viewport()->autoFillBackground());

        // Without WA_StyledBackground the page background rule is a no-op, which is
        // the single easiest thing in this redesign to leave out.
        auto *content = page.findChild<QWidget *>(QStringLiteral("pageContent"));
        QVERIFY(content);
        QVERIFY(content->testAttribute(Qt::WA_StyledBackground));

        m_settings->setLanguage(AppLanguage::SimplifiedChinese);
        page.retranslate();
        QCOMPARE(row->labelText(), m_localization->text(TextKey::LaunchAtLogin));
        m_settings->setLanguage(AppLanguage::English);
        page.retranslate();
    }

private:
    QTemporaryDir *m_temp = nullptr;
    AppSettings *m_settings = nullptr;
    Localization *m_localization = nullptr;
};

// Runs inside a grouped binary; see tests/support/TestRunner.h.
QObject *createUiKitTest() { return new UiKitTest; }
#include "test_ui_kit.moc"
