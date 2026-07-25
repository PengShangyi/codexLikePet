#pragma once

#include <QWidget>

class Localization;
class QLabel;
class QPushButton;

// App name, version (from the single CMake-sourced POTATO_VERSION), copyright,
// and a button that opens the bundled third-party license notices locally.
class AboutWindow final : public QWidget
{
    Q_OBJECT

public:
    explicit AboutWindow(Localization *localization, QWidget *parent = nullptr);

private:
    void retranslate();
    void openLicenses();

    Localization *m_localization;
    QLabel *m_name = nullptr;
    QLabel *m_version = nullptr;
    QLabel *m_tagline = nullptr;
    QLabel *m_copyright = nullptr;
    QPushButton *m_licensesButton = nullptr;
};
