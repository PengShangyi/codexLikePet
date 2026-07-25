#pragma once

#include <QFrame>

class QLabel;

// A tinted notice strip for import results and validation errors.
//
// Replaces a read-only QPlainTextEdit whose colour was switched with an inline
// setStyleSheet("QPlainTextEdit { color: #a02020; }") -- the repository's only
// stylesheet call site, and a hardcoded light-mode red. The severity is exposed as
// a Q_PROPERTY so the theme can select on it (InlineBanner[severity="error"])
// instead of anyone reaching for setStyleSheet again.
class InlineBanner final : public QFrame
{
    Q_OBJECT
    Q_PROPERTY(QString severity READ severityName)

public:
    enum class Severity { Info, Error };

    explicit InlineBanner(QWidget *parent = nullptr);

    // An empty message hides the banner, so a page can leave one permanently in
    // its layout without it taking up space until there is something to say.
    void setMessage(const QString &message, Severity severity);
    void clear();

    QString message() const;
    Severity severity() const;
    QString severityName() const;

private:
    QLabel *m_text;
    Severity m_severity = Severity::Info;
};
