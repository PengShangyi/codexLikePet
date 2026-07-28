#pragma once

#include <QString>

// Filesystem-safety rules shared by every code path that accepts pet resources:
// PetPackageValidator (a directory the user points at), ArchiveExtractor (a
// hostile .potatopet), and PetStore (an id used to build a path).
//
// These lived as four near-identical copies -- the control-character predicate
// alone was written out three times -- which is the failure mode this namespace
// exists to prevent: a rule tightened in one copy and missed in another is a
// silent hole, not a compile error. Callers keep their own error wording and any
// extra rule they need on top; the predicates here are the shared floor.
namespace PackagePolicy {

// C0 controls and DEL. Rejected anywhere in a package-relative path: they make
// paths unreviewable and are never legitimate in pet resources.
bool hasControlCharacters(QStringView text);

// A portable relative path: non-empty, no control characters, no backslash, no
// Windows drive letter, not absolute. On success writes QDir::cleanPath() of the
// input to *cleanPath. Says nothing about traversal -- see escapesRoot().
bool isPortableRelativePath(const QString &path, QString *cleanPath);

// True when an already-cleaned path climbs out of its root. Pair with
// isPortableRelativePath(); callers that also reject "." check for it themselves.
bool escapesRoot(const QString &cleanPath);

// Suffix allowlist for files inside a pet package, plus the license-file prefix
// exemption. Accepts a bare file name or a path (only the last segment matters).
bool isAllowedPackageFileName(const QString &path);

// Pet ids become directory names under the pets root, so they are restricted to
// a conservative lowercase set.
bool isValidPetId(const QString &id);

// A valid pet id derived from a display name, for prefilling the field rather
// than making the user work out the rule from a rejection. Lowercases, replaces
// every run of disallowed characters with a single hyphen, trims hyphens from the
// ends, and truncates to the length limit. Returns an empty string when nothing
// usable survives -- a name in a non-Latin script leaves the caller to ask for an
// id outright, which is honest, where a transliteration guess would not be.
QString suggestPetId(const QString &displayName);

}  // namespace PackagePolicy
