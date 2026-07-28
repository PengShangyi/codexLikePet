#pragma once

#include "pet/PetAtlas.h"

#include <QColor>
#include <QImage>
#include <QLatin1StringView>
#include <QRect>
#include <QString>
#include <QVector>

#include <array>

// Turns eleven horizontal animation strips into one Codex v2 atlas.
//
// This is the write-side twin of PetAtlas, and it exists because the assembly step
// was the one part of making a pet that Potato could not do: the guide had to hand
// it to the bundled Hatch Pet skill, which needs Python, Pillow, Codex and a
// terminal before the user can see their pet at all.
//
// Pure by construction -- no filesystem, no platform, no widgets, no mutable state.
// Decoding the strips belongs to the caller and writing the package belongs to
// PetPackageWriter, so everything here is deterministic and testable on synthetic
// images. Issues carry codes rather than sentences for the same reason: this layer
// links Qt Core and Gui only, and reaching for Localization would dissolve that
// boundary. The window turns each code into a TextKey.
namespace AtlasComposer {

// One row of the v2 grid. Only the *name* lives here: the frame count comes from
// PetAtlas::usedColumns(), which is the same function validateV2Occupancy enforces,
// so the composer cannot disagree with the validator that decides whether its output
// installs. The names match the contract in docs/PET_AUTHORING.md and the skill's
// references/animation-rows.md, because they are what the user puts in the prompt
// ("State: idle") and therefore what they name the file.
struct RowSpec {
    int row = 0;
    QLatin1StringView name;
};

const std::array<RowSpec, PetAtlas::Rows> &rows();

inline int frameCount(int row) { return PetAtlas::usedColumns(row); }

// The chroma green the guide's own prompt templates ask the model for. Deliberately
// not the skill's #00FF00: ours has to agree with what we tell the user to type.
QColor defaultChromaKey();

// The strip's top-left pixel. Every prompt demands a flat key filling the frame edge
// to edge, so the corner is background in any strip that followed instructions -- and
// in one that did not, the user can still set the key by hand.
QColor detectChromaKey(const QImage &strip);

// Distinct types rather than bools, because they sit beside an int in the options
// struct and `{key, 96, true, false}` reads as nothing at all.
enum class DespillEdges { Off, On };
enum class UpscaleFrames { Never, Allowed };

struct Options {
    QColor chromaKey = defaultChromaKey();
    // Chrominance distance, not RGB distance -- see keyToAlpha's definition for why
    // that choice is load-bearing rather than cosmetic. 0 keys nothing.
    int keyTolerance = 96;
    DespillEdges despill = DespillEdges::On;
    UpscaleFrames upscale = UpscaleFrames::Never;
};

// One authored row, already decoded. Rows are identified by index so a caller can
// hand over any subset and have the rest reported as missing rather than guessed at.
struct RowInput {
    int row = -1;
    QImage strip;
};

enum class Problem {
    // Blocking: the atlas cannot install.
    MissingRow,
    StripTooSmall,
    EmptyFrame,
    OccupancyFailed,
    // Advisory: the atlas installs, but it probably is not what the user wanted.
    UnexpectedStripAspect,
    OutlierFrame,
    FrameDoesNotFit,
};

bool isBlocking(Problem problem);

struct Issue {
    Problem problem;
    int row = -1;     // -1 when not row-specific
    int column = -1;  // -1 when not cell-specific
};

struct Result {
    // Produced even when incomplete, so a preview can show the user *why* it failed
    // rather than going blank. Only isInstallable() gates the install button.
    QImage atlas;
    QVector<Issue> issues;
    // The one scale applied to every frame, and the shared ground line. Surfaced
    // because a suspiciously low scale is the visible symptom of one oversized frame
    // shrinking all the others.
    double scale = 1.0;
    int baselineY = 0;

    bool isInstallable() const;
    bool hasBlockingIssue() const;
};

// The whole pipeline: slice, key, despill, measure, place, then self-verify against
// PetAtlas::validateV2Occupancy. Deterministic -- same inputs, same bytes out -- and
// it does not modify `inputs`.
Result compose(const QVector<RowInput> &inputs, const Options &options);

// Seams, so keying and measurement can be asserted without composing a whole atlas.
QImage keyToAlpha(const QImage &strip, const Options &options);
QRect alphaBounds(const QImage &image, const QRect &within);

}  // namespace AtlasComposer
